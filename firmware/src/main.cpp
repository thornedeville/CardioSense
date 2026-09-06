#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "buzzer.h"
#include "beat_detector.h"
#include "features.h"
#include "arrhythmia_model.h"

const uint32_t sampleIntervalUs = 1000000UL / SAMPLE_RATE_HZ;
uint32_t lastSampleTime = 0;

int currentBpm = 0; // updated live from real beat detection - drives display + buzzer

// Must match the label order emlearn printed in models/source/label_map.txt
const char* CLASS_LABELS[5] = {"F", "N", "Q", "S", "V"};
const int NORMAL_CLASS_INDEX = 1; // index of "N" in the array above

void setup() {
  Serial.begin(115200);

  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);
  analogReadResolution(12);

  displayInit();
  showStatus("READY");

  buzzerInit();
  beatDetectorInit();

  lastSampleTime = micros();
}

void loop() {
  uint32_t now = micros();
  if (now - lastSampleTime < sampleIntervalUs) return;
  lastSampleTime = now;

  bool leadsOff = digitalRead(LO_PLUS) || digitalRead(LO_MINUS);

  if (leadsOff) {
    Serial.println("LEADS_OFF");
    showStatus("LEADS OFF");
    return;
  }

  int raw = analogRead(ECG_PIN);
  Serial.println(raw);

  BeatDetectorResult result = beatDetectorUpdate(raw);

  if (result.newPeakDetected) {
    currentBpm = (int)round(result.instantBpm);
    showHeartRate(currentBpm);
  }

  if (result.beatReady) {
    int16_t features[FEATURE_COUNT];
    computeFeatures(result.beat.window, result.beat.rrPrevSec, result.beat.rrNextSec, features);

    int classIndex = arrhythmia_model_predict(features, FEATURE_COUNT);
    bool isAnomaly = (classIndex != NORMAL_CLASS_INDEX);
    const char* label = (classIndex >= 0 && classIndex < 5) ? CLASS_LABELS[classIndex] : "?";

    showAnomaly(isAnomaly, label);

    Serial.print("Beat classified: ");
    Serial.println(label);
  }

  heartbeatTick(currentBpm);
}