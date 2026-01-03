# Padium Pro - Hardware Plan & Assembly Guide

This document outlines the Bill of Materials (BOM) and the specific wiring configuration required to build the Padium Pro "Full Stage Edition".

## 📦 Bill of Materials (BOM)

| Component | Quantity | Description | Notes |
| :--- | :---: | :--- | :--- |
| **ESP32 DevKit V1** | 1 | MCU | WROOM-32 Module (30 or 38 pin version). |
| **PCM5102 DAC** | 1 | Audio Module | "Purple Board". **Solder SCK jumper to GND** on the front of the module. |
| **GC9A01 Display** | 1 | 1.28" Circular IPS LCD | SPI Interface. |
| **MicroSD Module** | 1 | Storage | SPI Interface (Standard Arduino module). |
| **Rotary Encoder** | 2 | Controls | KY-040 type. **Remove built-in pull-ups** if present on module, use external ones for Vol. |
| **Footswitch** | 3 | Controls | SPST Momentary (Soft Touch recommended). |
| **Resistor 10kΩ** | 3 | **CRITICAL** | Pull-ups for Volume Encoder (GPIO 34, 35, 39). |
| **Buck Converter** | 1 | Power Regulator | **MP1584EN** or **LM2596**. Steps 9V -> 5V. |
| **DC Jack** | 1 | Power Input | 2.1mm Center Negative (Standard Pedal Power). |
| **Jack P10 Mono** | 2 | Audio Out | TS (Tip-Sleeve) 6.35mm. For Left/Right Outputs. |
| **Jack P10 Stereo**| 1 | Audio Out | TRS (Tip-Ring-Sleeve) 6.35mm. For Headphones/Aux. |

---

## ⚡ Power Supply & Regulation

To avoid digital noise in the audio signal:

1.  **Source:** 9V DC (Center Negative).
2.  **Regulation:** Connect 9V to the Buck Converter Input.
3.  **Calibration:** Adjust the converter's trimpot until the Output is **5.1V**.
4.  **Distribution:**
    * Connect 5V Output to ESP32 `VIN` pin.
    * Connect 5V Output to PCM5102 `VIN` pin (Parallel power, do not draw from ESP32 3V3).
    * *Grounds:* Connect all grounds to a common point.

---

## 🔌 Pinout Configuration

### 1. Controls (Crucial Setup)

**⚠️ WARNING: INPUT-ONLY PINS**
The Volume Encoder uses GPIOs **34, 35, and 39**. These pins on the ESP32 are hardware-limited to "Input Only" and have **NO internal pull-up resistors**.
You **MUST** solder external 10kΩ resistors between these pins and the 3.3V rail. Without them, the volume knob will not work.

| Control | Function | ESP32 Pin | Wiring Note |
| :--- | :--- | :--- | :--- |
| **Nav Encoder** | DT (B) | **GPIO 17** | Internal Pull-up OK |
| | CLK (A) | **GPIO 16** | Internal Pull-up OK |
| | SW (Btn) | **GPIO 21** | Internal Pull-up OK |
| **Vol Encoder** | DT (B) | **GPIO 35** | **REQUIRES 10kΩ resistor to 3V3** |
| | CLK (A) | **GPIO 34** | **REQUIRES 10kΩ resistor to 3V3** |
| | SW (Back)| **GPIO 39** | **REQUIRES 10kΩ resistor to 3V3** |
| **Footswitches**| Prev | **GPIO 32** | Connect to GND when pressed |
| | Play | **GPIO 33** | Connect to GND when pressed |
| | Next | **GPIO 27** | Connect to GND when pressed |

### 2. Audio DAC (PCM5102)
| DAC Pin | ESP32 Pin |
| :--- | :--- |
| **BCK** | **GPIO 26** |
| **LRCK** | **GPIO 25** |
| **DIN** | **GPIO 22** |
| **SCK** | **GND** | (Jumper on DAC board) |
| **VIN** | **5V** | (From Regulator) |
| **GND** | **GND** | |

### 3. Display & SD Card
| Module | Pin | ESP32 Pin | Note |
| :--- | :--- | :--- | :--- |
| **Display** | MOSI | **GPIO 13** | |
| | SCLK | **GPIO 14** | |
| | CS | **GPIO 15** | |
| | DC | **GPIO 2** | |
| | RST | **EN** | Connect to ESP32 Reset/EN pin |
| | BLK | **GPIO 4** | Backlight PWM |
| **SD Card** | MOSI | **GPIO 23** | |
| | MISO | **GPIO 19** | |
| | SCK | **GPIO 18** | |
| | CS | **GPIO 5** | |

---

## 🎧 Audio Output Wiring (Full Pro)

This configuration provides 3 separate jacks connected in parallel.

**Jack 1: LEFT / MONO (Type: TS Mono)**
* **Tip:** Connect to PCM5102 **L**.
* **Sleeve:** Connect to Ground.

**Jack 2: RIGHT (Type: TS Mono)**
* **Tip:** Connect to PCM5102 **R**.
* **Sleeve:** Connect to Ground.

**Jack 3: STEREO / PHONES (Type: TRS Stereo)**
* **Tip:** Connect to PCM5102 **L** (or jump from Jack 1 Tip).
* **Ring:** Connect to PCM5102 **R** (or jump from Jack 2 Tip).
* **Sleeve:** Connect to Ground.

> **Operational Note:** Do not connect low-impedance headphones to the Stereo jack while simultaneously connected to a PA system via the Mono jacks. This can cause signal degradation due to impedance loading. Use one mode at a time.
