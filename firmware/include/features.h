#pragma once
#include <stdint.h>

#define FEATURE_COUNT   12
#define BEAT_WINDOW_LEN 250   // 0.4s before + 0.6s after the R-peak, at 250Hz
#define BEAT_LOOKBACK   100   // 0.4s at 250Hz
#define BEAT_LOOKAHEAD  150   // 0.6s at 250Hz

// Computes the same 12 features used in training, in the same order,
// scaled and clipped the same way, ready to feed directly into
// arrhythmia_model_predict().
//
// NOTE: the amplitude-based features were trained on wfdb's physical-unit
// signal values, not raw ADC counts. Until the real AD8232 is connected
// and calibrated against that scale, these features are numerically safe
// (no crashes, no overflow - just clipped at the int16 boundary) but not
// on the same scale the model learned from, so classification results are
// a structural placeholder until calibration happens.
void computeFeatures(const int* window, float rrPrevSec, float rrNextSec, int16_t* outFeatures);