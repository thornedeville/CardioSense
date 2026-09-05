#include <Arduino.h>
#include "arrhythmia_model.h"
#include "pins.h"
#include "display.h"
#include "buzzer.h"

const uint32_t sampleIntervalUs = 1000000UL / SAMPLE_RATE_HZ;
uint32_t lastSampleTime = 0;

int placeholderBpm = 72; // TODO: replace with real HR calculation

void setup() {
  Serial.begin(115200);

  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);
  analogReadResolution(12);

  displayInit();
  showStatus("READY");

  buzzerInit();

  int16_t dummyFeatures[12] = {0};
  arrhythmia_model_predict(dummyFeatures, 12);

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

  heartbeatTick(placeholderBpm);
}