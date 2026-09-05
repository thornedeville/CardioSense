#pragma once

// AD8232 wiring
#define ECG_PIN     34   // analog out from AD8232
#define LO_PLUS     32   // leads-off detect +
#define LO_MINUS    33   // leads-off detect -

#define SAMPLE_RATE_HZ 250

// LCD uses I2C - default ESP32 pins
// SDA = GPIO21, SCL = GPIO22 (wire it here, no code needed for this)

// Buzzer
#define BUZZER_PIN  25