#include <math.h>
#include "features.h"

static int16_t clampInt16(double value) {
  if (value > 32767) return 32767;
  if (value < -32768) return -32768;
  return (int16_t)value;
}

void computeFeatures(const int* window, float rrPrevSec, float rrNextSec, int16_t* outFeatures) {
  double sum = 0, sumSq = 0, sumCube = 0, sumQuad = 0;
  int minVal = window[0];
  int maxVal = window[0];
  int peakPos = 0;

  for (int i = 0; i < BEAT_WINDOW_LEN; i++) {
    double v = window[i];
    sum += v;
    sumSq += v * v;
    sumCube += v * v * v;
    sumQuad += v * v * v * v;

    if (window[i] < minVal) minVal = window[i];
    if (window[i] > maxVal) {
      maxVal = window[i];
      peakPos = i;
    }
  }

  double n = BEAT_WINDOW_LEN;
  double mean = sum / n;
  double variance = (sumSq / n) - (mean * mean);
  if (variance < 0) variance = 0;
  double stdDev = sqrt(variance);

  double m3 = (sumCube / n) - 3 * mean * (sumSq / n) + 2 * mean * mean * mean;
  double skew = (stdDev > 0.0001) ? (m3 / (stdDev * stdDev * stdDev)) : 0;

  double m4 = (sumQuad / n) - 4 * mean * (sumCube / n) + 6 * mean * mean * (sumSq / n) - 3 * mean * mean * mean * mean;
  double kurtosis = (variance > 0.0001) ? (m4 / (variance * variance)) - 3.0 : 0;

  double range = maxVal - minVal;
  double energy = sumSq;
  double rrRatio = (rrNextSec > 0.0001) ? (rrPrevSec / rrNextSec) : 0;

  outFeatures[0]  = clampInt16(round(mean     * 1000));
  outFeatures[1]  = clampInt16(round(stdDev   * 1000));
  outFeatures[2]  = clampInt16(round(minVal   * 1000));
  outFeatures[3]  = clampInt16(round(maxVal   * 1000));
  outFeatures[4]  = clampInt16(round(range    * 1000));
  outFeatures[5]  = clampInt16(round(skew     * 1000));
  outFeatures[6]  = clampInt16(round(kurtosis * 100));
  outFeatures[7]  = clampInt16(round(energy   * 0.5));
  outFeatures[8]  = clampInt16(peakPos);
  outFeatures[9]  = clampInt16(round(rrPrevSec * 1000));
  outFeatures[10] = clampInt16(round(rrNextSec * 1000));
  outFeatures[11] = clampInt16(round(rrRatio   * 400));
}