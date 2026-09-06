#include "beat_detector.h"

#define SAMPLE_RATE_HZ     250
#define SAMPLE_BUFFER_LEN  1000  // ~4s of history - comfortable margin for
                                   // the lookahead window plus queued beats
#define PENDING_QUEUE_SIZE 5

struct PendingBeat {
  uint32_t peakAbsIndex;
  float rrPrev;
  float rrNext;
  bool rrNextKnown;
};

static int circBuf[SAMPLE_BUFFER_LEN];
static uint32_t absoluteIndex = 0;

// Adaptive threshold, built from two independently-tracked pieces:
//  - baseline: a slow low-pass filter of the raw signal (resting/DC level)
//  - peakAmplitude: typical height of a real beat above baseline, updated
//    ONLY when a beat is confirmed - not continuously, which is what
//    caused false triggers during long pauses at slow heart rates before.
static float baseline;
static float peakAmplitude;
static const float BASELINE_LOWPASS = 0.001f;
static const float PEAK_AMPLITUDE_LOWPASS = 0.3f;
static const float THRESHOLD_FRACTION = 0.5f;

static const uint32_t REFRACTORY_SAMPLES = 50; // ~200ms - hearts can't beat faster than ~300bpm

static int prevSample;
static bool rising;
static uint32_t lastPeakAbsIndex;
static bool havePrevPeak;

static PendingBeat pendingQueue[PENDING_QUEUE_SIZE];
static int pendingHead;
static int pendingCount;

void beatDetectorInit() {
  absoluteIndex = 0;
  baseline = 2048;
  peakAmplitude = 500;
  prevSample = 2048;
  rising = false;
  lastPeakAbsIndex = 0;
  havePrevPeak = false;
  pendingHead = 0;
  pendingCount = 0;
}

static void pushPending(uint32_t peakAbsIndex, float rrPrev) {
  if (pendingCount >= PENDING_QUEUE_SIZE) {
    pendingHead = (pendingHead + 1) % PENDING_QUEUE_SIZE;
    pendingCount--;
  }
  int idx = (pendingHead + pendingCount) % PENDING_QUEUE_SIZE;
  pendingQueue[idx].peakAbsIndex = peakAbsIndex;
  pendingQueue[idx].rrPrev = rrPrev;
  pendingQueue[idx].rrNextKnown = false;
  pendingCount++;
}

static void buildWindow(uint32_t peakAbsIndex, int* outWindow) {
  uint32_t startAbs = peakAbsIndex - BEAT_LOOKBACK;
  for (int i = 0; i < BEAT_WINDOW_LEN; i++) {
    uint32_t abs = startAbs + i;
    outWindow[i] = circBuf[abs % SAMPLE_BUFFER_LEN];
  }
}

BeatDetectorResult beatDetectorUpdate(int rawSample) {
  BeatDetectorResult result = {};

  circBuf[absoluteIndex % SAMPLE_BUFFER_LEN] = rawSample;

  baseline += (rawSample - baseline) * BASELINE_LOWPASS;
  float threshold = baseline + THRESHOLD_FRACTION * peakAmplitude;

  if (absoluteIndex > (uint32_t)(BEAT_LOOKBACK + BEAT_LOOKAHEAD)) {
    bool wasRising = rising;
    rising = (rawSample > prevSample);

    if (wasRising && !rising && prevSample > threshold) {
      uint32_t peakAbsIndex = absoluteIndex - 1;

      if (!havePrevPeak || (peakAbsIndex - lastPeakAbsIndex) > REFRACTORY_SAMPLES) {
        for (int i = 0; i < pendingCount; i++) {
          int idx = (pendingHead + i) % PENDING_QUEUE_SIZE;
          if (!pendingQueue[idx].rrNextKnown) {
            pendingQueue[idx].rrNext = (peakAbsIndex - pendingQueue[idx].peakAbsIndex) / (float)SAMPLE_RATE_HZ;
            pendingQueue[idx].rrNextKnown = true;
          }
        }

        float rrPrev = havePrevPeak ? (peakAbsIndex - lastPeakAbsIndex) / (float)SAMPLE_RATE_HZ : 0;
        pushPending(peakAbsIndex, rrPrev);

        result.newPeakDetected = true;
        result.instantBpm = (rrPrev > 0.0001f) ? (60.0f / rrPrev) : 0;

        float thisPeakHeight = prevSample - baseline;
        if (thisPeakHeight > 0) {
          peakAmplitude += (thisPeakHeight - peakAmplitude) * PEAK_AMPLITUDE_LOWPASS;
        }

        lastPeakAbsIndex = peakAbsIndex;
        havePrevPeak = true;
      }
    }
  }

  if (pendingCount > 0) {
    PendingBeat &front = pendingQueue[pendingHead];
    if (front.rrNextKnown && absoluteIndex >= front.peakAbsIndex + BEAT_LOOKAHEAD + 1) {
      result.beatReady = true;
      result.beat.rrPrevSec = front.rrPrev;
      result.beat.rrNextSec = front.rrNext;
      buildWindow(front.peakAbsIndex, result.beat.window);

      pendingHead = (pendingHead + 1) % PENDING_QUEUE_SIZE;
      pendingCount--;
    }
  }

  prevSample = rawSample;
  absoluteIndex++;

  return result;
}