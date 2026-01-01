# Padium Pro

![Padium Pro Logo](./assets/logo.png)

**Standalone Hi-Fi Ambient Pad Player for Worship**

Padium Pro is a significant evolution of the Padium concept, advancing the work started in  [padium-mini-ino](https://github.com/henryhamon/padium-mini-ino) and [padium-mini](https://github.com/henryhamon/padium-mini). It transitions from a simple controller to a professional, standalone digital audio player designed specifically for live worship environments.

Built around the powerful ESP32 microcontroller, the Padium Pro eliminates the need for a laptop on stage. It delivers pristine, continuous ambient pads through a dedicated high-fidelity I2S DAC, controlled via an intuitive circular display and rugged footswitches.

## Key Features

*   **Standalone Operation:** Plays high-quality ambient pads directly from a microSD card. No computer required.
*   **Audiophile Sound:** Native 16-bit I2S output via PCM5102 DAC for noise-free, studio-quality audio.
*   **Professional Workflow:**
    *   **Queue & Confirm:** Select your next key while the current one plays, then seamlessly transition on cue.
    *   **Smart Crossfade:** Configurable transition engine (Crossfade or Hard Cut) with adjustable fade times (0-10s).
    *   **Chromatic Scale:** Full 12-key support (C, C#, D...) with automatic file name safety handling.
*   **Dual Encoder Interface:**
    *   **Primary Encoder:** Navigate Presets, Fade Times, and Settings.
    *   **Secondary Encoder:** Dedicated Master Volume control with a "Back/Exit" button for instant menu return.
*   **Robust System Design:**
    *   **Boot Diagnostics:** Self-tests SD card and file structure on startup to prevent mid-set failures.
    *   **Soft Stop:** Prevents abrupt audio cuts with an auto-fade logic.
    *   **Input Protection:** Potentiometer/Encoder logic features hysteresis and debounce to prevent parameter jitter.

## Hardware Overview

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
