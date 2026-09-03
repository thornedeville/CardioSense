# CardioSense

Real-time ECG signal acquisition and analysis using an ESP32 and AD8232 sensor, with on-device (or edge-assisted) model inference for live heart signal interpretation.

## Overview

CardioSense is an early-stage project aiming to build a low-cost, real-time ECG monitoring device. It combines an ESP32 microcontroller with an AD8232 ECG sensor module to capture heart signal data, with the goal of running analysis through a lightweight local model rather than relying on cloud processing.

This project is currently in the **planning and prototyping phase** — schematics and architecture are being worked out before hardware assembly and firmware development begin in earnest.

## Goals

- Capture clean ECG signal data using the AD8232 sensor
- Process and analyze signals in real time on (or near) the device
- Eventually produce a usable, standalone device — not just a lab prototype

## Hardware

- **Microcontroller:** ESP32
- **ECG Sensor:** AD8232
- *(Additional components to be added as the design is finalized)*

## Repository Structure

CardioSense/
├── firmware/ # ESP32 sketches (Arduino/C++)
├── hardware/
│ ├── schematics/ # Proteus circuit design files
│ └── previews/ # Exported images/PDFs of schematics
├── models/
│ ├── source/ # Editable 3D model files (.step, .blend, etc.)
│ └── renders/ # GitHub-viewable 3D files (.stl, .obj) + preview images
├── notebooks/ # Jupyter notebooks for signal analysis/experimentation
├── docs/ # Notes, references, writeups
└── README.md


## Status

🚧 **Early development** — no working prototype yet. Currently focused on circuit design and planning the signal processing pipeline. This README will be updated as the architecture and hardware setup are finalized.

## Getting Started

*(To be added once firmware and setup steps are in place.)*

## License

This project is licensed under the [MIT License](LICENSE).
