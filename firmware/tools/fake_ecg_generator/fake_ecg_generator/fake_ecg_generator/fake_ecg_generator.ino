// fake_ecg_generator.ino
// Stands in for the AD8232 - outputs a repeating heartbeat-shaped
// voltage on the DAC pin, so the main board has something to read.
//
// On every restart, picks a new random base heart rate (50-140 BPM) with
// small natural beat-to-beat variation. Most beats use a normal waveform
// shape, but occasionally (about 1 in every 15 beats) it sends a wider,
// PVC-like beat instead - arriving early (premature), with a different
// shape, followed by a longer pause (compensatory pause). This is the
// classic real-world pattern for a Ventricular ectopic beat, and gives
// Board A's classifier something genuinely different to catch, rather
// than only ever seeing identical normal beats.
//
// Ground truth for every beat is printed over Serial, so you can compare
// it directly against Board A's "Beat classified: X" output.

#include <esp_system.h>

const int DAC_PIN = 25;

// Roughly 1 in every N beats is sent as the abnormal (PVC-like) shape
const int ABNORMAL_BEAT_CHANCE_DENOM = 15;

// Normal beat: narrow, sharp spike - same shape used from the start
const uint8_t normalWaveform[50] = {
  128,128,129,131,133,131,129,128,128,128,
  128,120, 90,255, 40, 90,128,132,136,140,
  145,150,155,158,155,150,144,138,132,128,
  128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128
};

// PVC-like beat: wider, bizarre-looking QRS with a discordant dip
// afterward (crudely mimicking a real Ventricular ectopic beat's shape,
// not medically precise, just clearly distinguishable from Normal)
const uint8_t pvcWaveform[50] = {
  128,128,128,130,135,145,160,180,200,215,
  225,225,215,200,180,160,145,130,120,110,
  100, 90, 85, 80, 78, 80, 85, 90,100,110,
  120,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128
};

int baseBpm;
unsigned long lastSampleTime = 0;
unsigned long sampleIntervalUs;
int sampleIndex = 0;

const uint8_t* currentWaveform = normalWaveform;
bool lastBeatWasAbnormal = false;

unsigned long baseBeatIntervalUs() {
  // +-5% natural variation around the base rate
  long jitterPercent = (long)(esp_random() % 11) - 5; // -5 to +5
  long bpm = baseBpm + (baseBpm * jitterPercent) / 100;
  return 60000000UL / bpm;
}

unsigned long nextBeatIntervalUs() {
  unsigned long intervalUs = baseBeatIntervalUs();

  if (lastBeatWasAbnormal) {
    // Compensatory pause: the beat after a PVC-like beat typically
    // arrives later than usual, since the heart's normal rhythm was
    // briefly interrupted.
    intervalUs = (intervalUs * 130) / 100;
    lastBeatWasAbnormal = false;
  }

  bool thisBeatAbnormal = (esp_random() % ABNORMAL_BEAT_CHANCE_DENOM) == 0;

  if (thisBeatAbnormal) {
    // Premature: arrives earlier than the normal rhythm would predict
    intervalUs = (intervalUs * 70) / 100;
    currentWaveform = pvcWaveform;
  } else {
    currentWaveform = normalWaveform;
  }

  lastBeatWasAbnormal = thisBeatAbnormal;

  Serial.print("Sending: ");
  Serial.println(thisBeatAbnormal ? "PVC-like (wide QRS, premature)" : "Normal");

  return intervalUs;
}

void setup() {
  Serial.begin(115200);

  baseBpm = 50 + (esp_random() % 91); // random starting point: 50-140 BPM

  unsigned long beatIntervalUs = nextBeatIntervalUs();
  sampleIntervalUs = beatIntervalUs / 50;
  lastSampleTime = micros();

  Serial.print("Simulated heart rate this session: ~");
  Serial.print(baseBpm);
  Serial.println(" BPM (with natural variation and occasional PVC-like beats)");
}

void loop() {
  unsigned long now = micros();
  if (now - lastSampleTime >= sampleIntervalUs) {
    lastSampleTime = now;
    dacWrite(DAC_PIN, currentWaveform[sampleIndex]);
    sampleIndex++;

    if (sampleIndex >= 50) {
      sampleIndex = 0;
      unsigned long beatIntervalUs = nextBeatIntervalUs();
      sampleIntervalUs = beatIntervalUs / 50;
    }
  }
}
