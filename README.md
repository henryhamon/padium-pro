# Padium Pro

![Padium Pro Logo](./assets/logo.png)

**Standalone Hi-Fi Ambient Pad Player for Worship**

Padium Pro is the professional evolution of the open-source ambient pad controller. It transitions from a simple MIDI/Foot controller to a **standalone digital audio player** designed specifically for live worship environments.

Built around the powerful **ESP32**, Padium Pro eliminates the need for a laptop on stage. It delivers pristine, continuous ambient pads through a dedicated high-fidelity I2S DAC, controlled via a stunning circular display and rugged footswitches.

---

## 🚀 Key Features

### 🎧 Audiophile Sound Engine
* **Standalone Operation:** Plays MP3/WAV pads directly from a microSD card.
* **Hi-Fi Quality:** Native 16-bit I2S output via **PCM5102 DAC** for a noise-free, studio-quality noise floor (SNR > 112dB).
* **Smart Crossfade:** A dedicated RTOS Audio Task ensures seamless transitions between keys. Configurable fade times (0s - 10s) allow for smooth blending or instant cuts.

### 🎛 Professional Workflow
* **Queue & Confirm:** Browse and select the *Next Key* while the *Current Key* continues to play. Press play to transition on cue.
* **Chromatic Scale:** Full support for all 12 keys (C, C#, D...) with intelligent file name handling.
* **Panic Stop:** Long-press the Play button (>1s) to trigger a fast fade-out and silence the system immediately.
* **Dynamic Presets:** Organize your pads into folders (e.g., "Warm Pads", "Shimmer"). The system automatically scans and creates a list of banks on boot.

### 📡 Wi-Fi File Manager
Stop removing the SD card. Padium Pro creates its own Wi-Fi Hotspot:
* **Web Interface:** Manage files from your phone or laptop.
* **Full Control:** Create new Preset Banks (folders), upload MP3s wirelessly, and recursively delete old content.
* **Safe Mode:** Audio playback stops automatically during Wi-Fi operations to prevent errors.

### 🖥 Visuals & UI
* **Circular Interface:** Optimized for GC9A01 round displays.
* **Themes:** Switch between **Dark Mode** (Stage) and **Light Mode** (Daytime/Studio).
* **Screensaver:** Auto-dims the backlight after 30 seconds of inactivity to prevent distraction and extend screen life.

---

## 🎮 Controls Overview

The Padium Pro uses a "Dual Encoder" philosophy for fast access:

| Control | Action | Function |
| :--- | :--- | :--- |
| **Nav Encoder** | Rotate | Change Preset Bank (or scroll menus) |
| | Click | Enter/Select in **Settings Menu** |
| **Vol Encoder** | Rotate | **Master Volume** (Global) |
| | Click | **Back / Return** (Instant exit from menus) |
| **Footswitch 1** | Press | Previous Key |
| | Hold | Fast Scroll / Whole Tone Jump |
| **Footswitch 2** | Press | **Play / Transition / Stop** |
| | Hold | **Panic Stop** (Kill Audio) |
| **Footswitch 3** | Press | Next Key |
| | Hold | Fast Scroll / Whole Tone Jump |

---

## 🛠 Software Architecture

The firmware uses **FreeRTOS** to guarantee audio stability:

* **Core 0 (Audio Task):** Dedicated high-priority task for decoding MP3s and feeding the I2S DAC. Uses Mutexes to safely access the SD card.
* **Core 1 (UI & Logic):** Handles the display, button debouncing (`InputManager`), and Wi-Fi networking (`WifiManager`).
* **Chunked Streaming:** The Web Server uses chunked transfer encoding, allowing it to list thousands of files without crashing the ESP32's memory.

---

## 🤝 Collaboration

The Padium Pro is an open platform. We encourage the community to contribute:
* **Code:** Submit PRs for new features or optimizations.
* **Hardware:** Design custom PCBs or 3D printed enclosures.
* **Share:** Tag us in your builds!

## 📜 License

This project is open source.

* **Firmware:** Released under the [GNU General Public License v3.0](LICENSE).
* **Hardware Design:** Released under the [CERN Open Hardware Licence v2](https://ohwr.org/project/cernohl/wikis/Documents/CERN-OHL-version-2).
* **Documentation:** [CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/).

---
*Created by [@henryhamon](https://github.com/henryhamon)*