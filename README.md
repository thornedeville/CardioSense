# CardioSense

A real-time ECG acquisition and multi-class arrhythmia detection device, built on an ESP32 and an AD8232 heart sensor, with on-device machine learning inference.

## Overview

CardioSense samples a single-lead ECG signal at 250Hz, detects individual heartbeats in real time, extracts a set of statistical features from each beat, and classifies each one using a Random Forest model running directly on the ESP32 - no cloud calls, no external inference server. The model is trained on three combined PhysioNet arrhythmia databases and classifies beats into the five AAMI-standard categories: Normal, Supraventricular, Ventricular, Fusion, and Unknown/Paced.

Output is shown on a 16x2 LCD and an active buzzer, both of which reflect the live, confirmed classification state.

## Current status

**Working end-to-end:**
- ESP32 firmware sampling at 250Hz with leads-off detection
- Real-time R-peak detection and BPM calculation from live signal
- Feature extraction matching the exact pipeline used in training
- On-device Random Forest inference via a compact C header (no ML runtime library required)
- LCD status/heart-rate/anomaly display, debounced against single-beat misclassifications
- Buzzer feedback timed to live BPM
- Two-board test rig (see below) for development without physical AD8232 hardware

**Not yet done:**
- Real AD8232 hardware has not been acquired yet - all testing so far uses a second ESP32 simulating the sensor's output
- Feature scaling is calibrated for PhysioNet's physical-unit (mV-scale) signal data, not raw ESP32 ADC counts - real hardware will need a calibration pass before classifications are clinically meaningful (see Known Limitations)

## Hardware

| Component | Status |
|---|---|
| ESP32 (main board) | In use |
| AD8232 ECG sensor | Not yet acquired - simulated via a second ESP32 (see below) |
| 16x2 I2C LCD | Wired and working |
| Active buzzer | Wired and working |

### Temporary two-board test setup

With no physical AD8232 available yet, a second ESP32 ("Board B") simulates the sensor by outputting a repeating heartbeat-shaped waveform via its onboard DAC. Board B picks a new random base heart rate (50-140 BPM) with small natural beat-to-beat variation on every restart, so the firmware can be exercised against a range of realistic timings.

The simulator sketch lives at `firmware/tools/fake_ecg_generator/fake_ecg_generator.ino` and is kept in the repo for anyone else developing ESP32 ECG firmware without hardware on hand yet.

**Wiring:**

| From | Pin | To | Pin | Purpose |
|---|---|---|---|---|
| Board B | GPIO25 (DAC out) | Board A | GPIO34 (`ECG_PIN`) | Simulated ECG signal |
| Board A | VIN | Board B | VIN | Shared power |
| Board A | GND | Board B | GND | Shared ground (required) |
| Board A | GPIO32 | Board A | GND | `LO_PLUS` override (leads-off jumper) |
| Board A | GPIO33 | Board A | GND | `LO_MINUS` override (leads-off jumper) |
| LCD (I2C, addr 0x27) | SDA / SCL | Board A | GPIO21 / GPIO22 | Display |
| LCD (I2C, addr 0x27) | VCC / GND | Board A | 3.3V or 5V / GND | Display power (check your module's rating) |
| Buzzer | + / - | Board A | GPIO25 / GND | Audible feedback |

The `LO_PLUS`/`LO_MINUS` jumper-to-GND is only needed because there's no real AD8232 driving those pins yet - remove it once real hardware is connected, since the AD8232 drives those pins itself.

## Repository structure

```
firmware/           PlatformIO project - ESP32 firmware
  include/            Headers (pins, display, buzzer, beat detector, feature extraction, model)
  src/                 Implementation files
  tools/               Development-only tools (fake ECG generator)
hardware/
  schematics/         Circuit schematics (pending - no physical AD8232 wiring finalized yet)
  previews/           Schematic/board preview images
models/
  source/             Trained model artifacts (arrhythmia_model.h, label_map.txt) - committed, used directly by firmware
  renders/            Physical enclosure renders (pending)
notebooks/            ML pipeline - data loading, feature engineering, training, export
docs/                 Additional documentation
```

## Machine learning pipeline

**Data:** Three PhysioNet databases combined to reduce class imbalance:
- MIT-BIH Arrhythmia Database (`mitdb`) - 360Hz, the primary dataset
- MIT-BIH Supraventricular Arrhythmia Database (`svdb`) - 128Hz, adds Supraventricular examples
- St Petersburg INCART 12-lead Arrhythmia Database (`incartdb`) - 257Hz, adds Ventricular examples

Only the first channel of each recording is used, matching the single-lead AD8232 hardware setup.

**Features:** Each heartbeat is reduced to 12 features - amplitude statistics (mean, std, min, max, range, skew, kurtosis, energy), peak position within the window, and R-R interval timing (previous interval, next interval, and their ratio) - rather than feeding raw waveform samples into the model.

**Model:** Random Forest (scikit-learn), 30 trees, max depth 10, class-balanced weighting. Exported to a C header via [emlearn](https://github.com/emlearn/emlearn) using its `loadable` method, which keeps the compiled footprint small enough to run comfortably on the ESP32.

**Validated accuracy** (on held-out test data, confirmed against the actual compiled C model, not just the Python version):

| Class | Precision | Recall | F1 |
|---|---|---|---|
| Normal | 0.99 | 0.91 | 0.95 |
| Ventricular | 0.79 | 0.88 | 0.84 |
| Unknown/Paced | 0.63 | 0.92 | 0.75 |
| Supraventricular | 0.45 | 0.87 | 0.59 |
| Fusion | 0.09 | 0.78 | 0.16 |

Overall accuracy: 91%.

**To retrain:** see `notebooks/02_train_baseline_model.ipynb`. Requires the three databases downloaded locally (recommended: `aws s3 sync --no-sign-request s3://physionet-open/<db>/1.0.0/ data/<db>/` for each), see `requirements.txt` for the Python environment.

## Firmware setup

Built with [PlatformIO](https://platformio.org/).

```bash
cd firmware
pio run                  # build only
pio run --target upload  # build and flash
pio device monitor       # view Serial output
```

Required libraries (`marcoschwartz/LiquidCrystal_I2C`) install automatically via `platformio.ini`. The model's runtime dependencies (`eml_trees.h`, `eml_common.h`, `eml_log.h`) must be copied manually from your local `emlearn` Python package installation into `firmware/include/` alongside `arrhythmia_model.h` - these are small (under 20KB combined) but required for the `loadable` export method to compile.

## Known limitations

- **Feature scale calibration:** the model's amplitude-based features were trained on PhysioNet's physical-unit (mV-scale) signal values. Live ESP32 ADC readings are raw 12-bit counts on a different scale. The current firmware computes structurally correct features from real signal data, but classification will not be clinically meaningful until a calibration step is done against real AD8232 hardware.
- **Fusion beat precision is low (~9%).** Fusion beats made up a very small fraction of the training data (a few hundred examples across all three databases) and are inherently ambiguous, being a blend of two other beat morphologies. The model catches most real Fusion beats (78% recall) but also produces a fair number of false positives for this specific class. Other classes (Normal, Ventricular, Unknown, Supraventricular) are substantially more reliable.
- **Display debouncing:** the on-screen anomaly status requires 3 consecutive matching beat classifications before updating, to filter out single-beat noise (e.g. startup transients). This means the display lags true state by a few heartbeats and will not flag a genuine one-off abnormal beat as an alert by design.

## License

MIT - see `LICENSE`.

## Data acknowledgment

This project uses data from PhysioNet:

Goldberger AL, Amaral LAN, Glass L, Hausdorff JM, Ivanov PCh, Mark RG, Mietus JE, Moody GB, Peng C-K, Stanley HE. PhysioBank, PhysioToolkit, and PhysioNet: Components of a New Research Resource for Complex Physiologic Signals. *Circulation* 101(23):e215-e220, 2000.

- MIT-BIH Arrhythmia Database: Moody GB, Mark RG. The impact of the MIT-BIH Arrhythmia Database. *IEEE Eng in Med and Biol* 20(3):45-50, 2001.
- MIT-BIH Supraventricular Arrhythmia Database: Greenwald SD. Development and analysis of a cardiac arrhythmia detector. MIT Dept. of Electrical Engineering and Computer Science, 1990.
- St Petersburg INCART 12-lead Arrhythmia Database, hosted via PhysioNet.
