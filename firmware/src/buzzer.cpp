#include <Arduino.h>
#include "pins.h"
#include "buzzer.h"

const uint32_t beepDurationMs = 60;

uint32_t lastBeatTime = 0;
bool buzzerOn = false;

void buzzerInit() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void heartbeatTick(int bpm) {
  if (bpm <= 0) return; // no valid reading yet, stay silent

  uint32_t beatIntervalMs = 60000UL / bpm;
  uint32_t now = millis();

  // time to start a new beep
  if (!buzzerOn && (now - lastBeatTime >= beatIntervalMs)) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOn = true;
    lastBeatTime = now;
  }

  // time to stop the current beep
  if (buzzerOn && (now - lastBeatTime >= beepDurationMs)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}