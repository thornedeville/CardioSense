// fake_ecg_generator.ino
// Stands in for the AD8232 - outputs a repeating heartbeat-shaped
// voltage on the DAC pin, so the main board has something to read.
//
// On every restart, picks a new random-ish base heart rate (50-140 BPM),
// then adds small natural beat-to-beat variation on top of it - real
// hearts don't beat at a perfectly fixed interval, so this is closer
// to what a genuine signal looks like than a constant, robotic rate.

#include <esp_system.h>

const int DAC_PIN = 25;

// One simplified heartbeat cycle, 50 points, values 0-255 (0-3.3V)
const uint8_t ecgWaveform[50] = {
  128,128,129,131,133,131,129,128,128,128,
  128,120, 90,255, 40, 90,128,132,136,140,
  145,150,155,158,155,150,144,138,132,128,
  128,128,128,128,128,128,128,128,128,128,
  128,128,128,128,128,128,128,128,128,128
};

int baseBpm;
unsigned long lastSampleTime = 0;
unsigned long sampleIntervalUs;
int sampleIndex = 0;

unsigned long randomBeatIntervalUs() {
  // +-5% natural variation around the base rate - a simple stand-in
  // for real heart-rate variability (HRV)
  long jitterPercent = (long)(esp_random() % 11) - 5; // -5 to +5
  long bpm = baseBpm + (baseBpm * jitterPercent) / 100;
  return 60000000UL / bpm;
}

void setup() {
  Serial.begin(115200);

  // esp_random() is the ESP32's built-in hardware random number generator -
  // genuinely different every boot, no manual seeding needed.
  baseBpm = 50 + (esp_random() % 91); // random starting point: 50-140 BPM

  unsigned long beatIntervalUs = randomBeatIntervalUs();
  sampleIntervalUs = beatIntervalUs / 50;
  lastSampleTime = micros();

  Serial.print("Simulated heart rate this session: ~");
  Serial.print(baseBpm);
  Serial.println(" BPM (with natural beat-to-beat variation)");
}

void loop() {
  unsigned long now = micros();
  if (now - lastSampleTime >= sampleIntervalUs) {
    lastSampleTime = now;
    dacWrite(DAC_PIN, ecgWaveform[sampleIndex]);
    sampleIndex++;

    if (sampleIndex >= 50) {
      sampleIndex = 0;
      // recompute timing for the next beat - this is what makes the
      // rate wander slightly instead of staying perfectly fixed
      unsigned long beatIntervalUs = randomBeatIntervalUs();
      sampleIntervalUs = beatIntervalUs / 50;
    }
  }
}