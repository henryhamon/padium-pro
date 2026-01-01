# Padium Pro

![Padium Pro Logo](./assets/logo.png)

**Standalone Hi-Fi Ambient Pad Player for Worship**

Padium Pro is a significant evolution of the Padium concept, advancing the work started in [padium-mini-ino](https://github.com/henryhamon/padium-mini-ino) and [padium-mini](https://github.com/henryhamon/padium-mini). It transitions from a simple controller to a professional, standalone digital audio player designed specifically for live worship environments.

Built around the powerful ESP32 microcontroller, the Padium Pro eliminates the need for a laptop on stage. It delivers pristine, continuous ambient pads through a dedicated high-fidelity I2S DAC, controlled via an intuitive circular display and rugged footswitches.

## Key Features

*   **Standalone Operation:** Plays high-quality ambient pads directly from a microSD card. No computer required.
*   **Audiophile Sound:** Native 16-bit I2S output via PCM5102 DAC for noise-free, studio-quality audio.
*   **Dynamic Presets:** Automatically scans the SD card for Bank folders. Create new Banks and upload files wirelessly.
*   **Professional Workflow:**
    *   **Queue & Confirm:** Select your next key while the current one plays, then seamlessly transition on cue.
    *   **Smart Crossfade:** Configurable transition engine (Crossfade or Hard Cut) with adjustable fade times (0-10s).
    *   **Chromatic Scale:** Full 12-key support (C, C#, D...) with automatic file name safety handling.
    *   **Panic Stop:** Long-press Play (>1s) to immediately fade out and stop audio.
*   **Wi-Fi File Manager:**
    *   Built-in Web Server for managing files without removing the SD card.
    *   Upload MP3s directly to specific Bank folders.
    *   Create new Banks and recursively delete entire folders.
*   **Dual Encoder Interface:**
    *   **Primary Encoder:** Navigate Presets, Fade Times, and Settings.
    *   **Secondary Encoder:** Dedicated Master Volume control with a "Back/Exit" button for instant menu return.
*   **Robust System Design:**
    *   **Boot Diagnostics:** Self-tests SD card and file structure on startup to prevent mid-set failures.
    *   **Screensaver:** Auto-dims display after 30s to prevent burn-in and distractions.
    *   **Input Protection:** Potentiometer/Encoder## Software Features

### 1. Advanced Audio Engine
- **Crossfading**: Smoothly transitions between pad keys (configurable 0-10s).
- **Soft Stop**: Gently fades out audio when stopping, preventing abrupt cuts.
- **Panic Mode**: Long-press "Play/Stop" to immediately kill all audio (with a fast fade).
- **Mutex Protection**: Ensures audio playback is never interrupted by Wi-Fi operations (`portMAX_DELAY` safety).

### 2. Dynamic Preset Management
- **Unlimited Presets**: Just add folders to the SD card root (e.g., `Ambient 1`, `Worship Pads`).
- **Auto-Scanning**: The device automatically scans and sorts folders on boot.
- **Wi-Fi Manager**: Create new banks, upload mp3s, and delete files wirelessly via a built-in web interface.
  - **Streaming Web Server**: Optimized "Chunked Transfer" listing to handle thousands of files without crashing.

### 3. Professional UI/UX
- **OLED/TFT Display**: Shows current key, next key, preset name, volume, and settings.
- **Screensaver**: Auto-dims the display after 30s of inactivity to prevent burn-in.
- **Theme Support**: Switch between Dark Mode (Stage) and Light Mode (Studio).
- **Intuitive Menu**: Adjust Fade Time, Crossfade (On/Off), Brightness, and Wi-Fi Mode.

## Architecture

The firmware has been refactored for professional-grade stability and maintainability:

- **InputManager**: Encapsulates all encoder and button logic (Debounce, Hold detection, Input-Only pin safety).
- **SettingsManager**: Handles persistent storage (NVS) for volume, brightness, and theme preferences.
- **WifiManager**: Encapsulates the Web Server and SoftAP logic, keeping the main loop clean.
- **AudioTask**: Runs on Core 0 (High Priority) to ensure skip-free playback.
- **Safety Specifics**:
  - **Thread-Safe**: Uses FreeRTOS Mutexes to protect SD card access.
  - **Memory-Safe**: Uses `snprintf` and `strncpy` to prevent buffer overflows.
  - **Crash-Proof**: Web server uses streaming to keep RAM usage low.

## History & Evolution

This project is the professional evolution of the Padium series:
1.  **Padium Mini (Arduino)**: The original proof of concept. https://github.com/henryhamon/padium-mini-ino
2.  **Padium Mini (PlatformIO)**: Variable architecture. https://github.com/henryhamon/padium-mini
3.  **Padium Pro (Current)**: ESP32-based, RTOS-driven, with Wi-Fi and TFT support.

## Hardware Requirements

- **MCU**: ESP32 Dev Module (WROOM-32)
- **Audio**: MAX98357A I2S Amp
- **Display**: ST7789 TFT (240x240)
- **Controls**:
  - Rotary Encoder (Navigation) + Button
  - Rotary Encoder (Volume) + Button
  - 2x Footswitches (Next/Prev)
  - 1x Play/Stop Button
- **Storage**: MicroSD Card Module (VSPI)

## Pinout Configuration

See `src/Config.h` for the exact pin definitions. **Note**: Pins GPIO 34, 35, 36, and 39 are input-only on the ESP32 and do not support internal pull-ups; external 10k pull-up resistors are required for buttons connected to these pins.
 Padium Pro does), you **MUST** use external 10k pull-up resistors to 3.3V. Internal pull-ups are not available on these pins.

## Menu System

Press the **Navigation Encoder Button** to enter the Settings Menu.
*   **Fade Time:** Adjust crossfade duration (0s - 10s).
*   **Transition:** Toggle between Crossfade (Smooth) and Cut (Instant).
*   **Theme:** Switch between Day (Light) and Night (Dark) UI modes.
*   **Brightness:** Adjust screen backlight intensity.
*   **Wi-Fi Mgr:** Activates the Wireless File Manager (SoftAP Mode).

## Hardware Awarerness

The Padium Pro is designed to be easily replicable by the DIY community.

*   **MCU:** ESP32-WROOM-32 DevKit V1
*   **DAC:** PCM5102 (I2S interface, "purple board" version)
*   **Display:** 1.28" Circular TFT LCD (GC9A01 driver, SPI)
*   **Controls:**
    *   3x Footswitches (Prev, Play/Stop, Next)
    *   1x Rotary Encoder (Navigation + Select)
    *   1x Rotary Encoder (Volume + Back/Exit)
*   **Storage:** MicroSD Card Module (SPI)
*   **Power:** 9V DC Input -> MP1584 Buck Converter (5V).

**Important Pin Note:**
Pins GPIO 34, 35, 36, and 39 are **Input Only** on the ESP32. If using these for buttons or encoders (as Padium Pro does), you **MUST** use external 10k pull-up resistors to 3.3V. Internal pull-ups are not available on these pins.

## Collaboration

The Padium Pro is an open platform for musicians and engineers. We encourage community involvement to refine the experience:

*   **Firmware Improvements:** Optimization of the audio stack, new UI themes, or additional features.
*   **Hardware Mods:** Custom PCB designs, alternative enclosures, or pedalboard integration ideas.
*   **Share Your Build:** Post photos of your completed Padium Pro and how you use it in your setup.

**Elevate Your Sound**

Padium Pro provides the stability, quality, and simplicity needed for live performance, ensuring your ambient soundscapes are always ready when you need them.

## License

This project is open source.

*   **Firmware:** Released under the [GNU General Public License v3.0](LICENSE).
*   **Hardware (Schematics & PCB):** Released under the [CERN Open Hardware Licence v2 - Strongly Reciprocal](https://ohwr.org/project/cernohl/wikis/Documents/CERN-OHL-version-2).
*   **Documentation & Artwork:** Released under [Creative Commons Attribution-ShareAlike 4.0 International](http://creativecommons.org/licenses/by-sa/4.0/).

Created by [@henryhamon](https://github.com/henryhamon).
