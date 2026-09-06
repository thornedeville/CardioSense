#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "buzzer.h"
#include "beat_detector.h"
#include "features.h"
#include "arrhythmia_model.h"

const uint32_t sampleIntervalUs = 1000000UL / SAMPLE_RATE_HZ;
uint32_t lastSampleTime = 0;

int currentBpm = 0;

const char* CLASS_LABELS[5] = {"F", "N", "Q", "S", "V"};
const char* CLASS_NAMES[5]  = {"Fusion", "Normal", "Paced/Unk", "SupraVent", "Ventric"};
const int NORMAL_CLASS_INDEX = 1;

const int CONFIRM_THRESHOLD = 3;

int candidateClass = -1;
int candidateCount = 0;
int confirmedClass = NORMAL_CLASS_INDEX;
bool hasShownConfirmedState = false; // forces the first real reading onto the display

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

    Serial.print("Beat classified: ");
    Serial.println(CLASS_LABELS[classIndex]);

    if (classIndex == candidateClass) {
      candidateCount++;
    } else {
      candidateClass = classIndex;
      candidateCount = 1;
    }

    bool stateChanged = (candidateClass != confirmedClass);
    if (candidateCount >= CONFIRM_THRESHOLD && (stateChanged || !hasShownConfirmedState)) {
      confirmedClass = candidateClass;
      hasShownConfirmedState = true;
      bool isAnomaly = (confirmedClass != NORMAL_CLASS_INDEX);
      showAnomaly(isAnomaly, CLASS_NAMES[confirmedClass]);

      Serial.print("Confirmed state changed to: ");
      Serial.println(CLASS_NAMES[confirmedClass]);
    }
  }

  heartbeatTick(currentBpm);
}