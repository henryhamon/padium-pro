# Padium Pro

![Padium Pro Logo](./assets/logo_pro.png)

**Standalone Hi-Fi Ambient Pad Player for Worship**

Padium Pro is a significant evolution of the Padium concept. It transitions from a simple controller to a professional, standalone digital audio player designed specifically for live worship environments.

Built around the powerful ESP32 microcontroller, the Padium Pro eliminates the need for a laptop on stage. It delivers pristine, continuous ambient pads through a dedicated high-fidelity I2S DAC, controlled via an intuitive circular display and rugged footswitches.

## Key Features

* **Standalone Operation:** No computer required. Plays high-quality ambient pads directly from a microSD card.
* **Audiophile Sound Quality:** Utilizes a dedicated PCM5102 I2S DAC module for noise-free, 16-bit high-fidelity audio output, vastly superior to standard PWM audio.
* **Visual Interface:** Features a stunning 1.28" circular TFT display (GC9A01) that provides clear, immediate visual feedback on the current key, next key, volume, and preset bank.
* **Seamless Transitions (Smart Crossfade):** Engineered for smooth, glitch-free audio. Changing keys triggers an automatic, customizable crossfade, ensuring continuous sound without abrupt cuts.
* **Preset Banks:** Organize your pads into folders (banks) on the SD card and navigate between them easily using the onboard menu.
* **Professional Controls:**
    * 3x Robust Footswitches (Previous Key, Play/Stop, Next Key).
    * 1x Analog Potentiometer for immediate master volume control.
    * 1x Rotary Encoder with Push Button for menu navigation and setting adjustments (e.g., Fade Time).
* **Open Source Hardware & Firmware:** Built on the accessible ESP32 platform using the Arduino framework. Modify the code, customize the interface, or build your own hardware.

## Hardware Overview

The Padium Pro is designed to be easily replicable by the DIY community.

* **MCU:** ESP32-WROOM-32 DevKit V1
* **DAC:** PCM5102 (I2S interface, "purple board" version recommended)
* **Display:** 1.28" Circular TFT LCD (GC9A01 driver, SPI interface)
* **Storage:** Standard MicroSD Card Module (SPI interface)
* **Power:** 9V DC Input fed into an MP1584 Buck Converter (regulated to 5V for the ESP32 VIN).

## Collaboration

The Padium Pro is an open platform for musicians and engineers. We encourage community involvement to refine the experience:

* **Firmware Improvements:** Optimization of the audio stack, new UI themes, or additional features.
* **Hardware Mods:** Custom PCB designs, alternative enclosures, or pedalboard integration ideas.
* **Share Your Build:** Post photos of your completed Padium Pro and how you use it in your setup.

**Elevate Your Sound**

Padium Pro provides the stability, quality, and simplicity needed for live performance, ensuring your ambient soundscapes are always ready when you need them.

## License

This project is open source.

* **Firmware:** Released under the [GNU General Public License v3.0](LICENSE).
* **Hardware (Schematics & PCB):** Released under the [CERN Open Hardware Licence v2 - Strongly Reciprocal](https://ohwr.org/project/cernohl/wikis/Documents/CERN-OHL-version-2).
* **Documentation & Artwork:** Released under [Creative Commons Attribution-ShareAlike 4.0 International](http://creativecommons.org/licenses/by-sa/4.0/).

Designed by (@henryhamon)[https://github.com/henryhamon].
