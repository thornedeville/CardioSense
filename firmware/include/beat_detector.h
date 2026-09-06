#pragma once
#include <stdint.h>
#include "features.h"

struct DetectedBeat {
  int window[BEAT_WINDOW_LEN];
  float rrPrevSec;
  float rrNextSec;
};

struct BeatDetectorResult {
  bool newPeakDetected;   // a fresh R-peak was just found this sample
  float instantBpm;       // valid only when newPeakDetected is true - low latency, for display

  bool beatReady;          // an earlier beat's full window + rr_next are now known
  DetectedBeat beat;       // valid only when beatReady is true - feed this to computeFeatures()
};

void beatDetectorInit();
BeatDetectorResult beatDetectorUpdate(int rawSample);