# Padium Pro - Hardware Plan & Assembly Guide

This document outlines the Bill of Materials (BOM) and the specific wiring configuration required to build the Padium Pro.

## 📦 Bill of Materials (BOM)

| Component | Quantity | Description | Notes |
| :--- | :---: | :--- | :--- |
| **ESP32 DevKit V1** | 1 | MCU | WROOM-32 Module (30 or 38 pin version) |
| **PCM5102 DAC** | 1 | Audio Module | "Purple Board" recommended. **Solder SCK jumper to GND** on the module. |
| **GC9A01 Display** | 1 | 1.28" Circular IPS LCD | SPI Interface. |
| **MicroSD Module** | 1 | Storage | SPI Interface (Standard Arduino module). |
| **Rotary Encoder** | 2 | Controls | KY-040 type or plain encoder. |
| **Footswitch** | 3 | Controls | SPST Momentary (Soft Touch recommended). |
| **Resistor 10kΩ** | 3 | Pull-ups | **CRITICAL:** For GPIO 34, 35, 39. |
| **Power Regulator** | 1 | Buck Converter | **MP1584EN** (Small) OR **LM2596** (Standard). Converts 9V -> 5V. |
| **DC Jack** | 1 | Power | 2.1mm Center Negative (Standard Pedal Power). |
| **Audio Jack** | 2 | Audio Out | 6.35mm (1/4") Mono or Stereo Jacks. |

---

## 🔌 Pinout Configuration

The firmware is pre-configured for the following pin map.
**⚠️ CAUTION:** The ESP32 has specific limitations on certain pins (Input Only, Strap Pins). Follow this guide strictly.

### 1. I2S Audio DAC (PCM5102)
| DAC Pin | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **BCK** | **GPIO 26** | Bit Clock |
| **LRCK** | **GPIO 25** | Word Select (L/R Clock) |
| **DIN** | **GPIO 22** | Data In |
| **SCK** | **GND** | *Hardwire to GND on DAC module* |
| **VIN** | **5V** | External 5V (Clean power preferred) |
| **GND** | **GND** | Common Ground |

### 2. MicroSD Card (VSPI)
| SD Pin | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **CS** | **GPIO 5** | Chip Select |
| **MOSI** | **GPIO 23** | Master Out Slave In |
| **MISO** | **GPIO 19** | Master In Slave Out |
| **SCK** | **GPIO 18** | Clock |

### 3. Display GC9A01 (HSPI)
| Display Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **MOSI (SDA)**| **GPIO 13** | |
| **SCLK (SCL)**| **GPIO 14** | |
| **CS** | **GPIO 15** | |
| **DC** | **GPIO 2** | |
| **RST** | **EN / 3V3** | *Connect to ESP32 EN pin or 3.3V. Do NOT use GPIO 4.* |
| **BLK** | **GPIO 4** | PWM Brightness Control |

### 4. Controls (Encoders & Footswitches)

**⚠️ CRITICAL WARNING:**
GPIOs **34, 35, and 39** are **Input Only** and do NOT have internal pull-up resistors. You **MUST** solder physical 10kΩ resistors between these pins and 3.3V.

| Control | Function | ESP32 Pin | Component Pin | Pull-up Req? |
| :--- | :--- | :--- | :--- | :--- |
| **Encoder 1** | Navigation A | **GPIO 16** | CLK | Internal OK |
| (Nav) | Navigation B | **GPIO 17** | DT | Internal OK |
| | Menu / Select | **GPIO 21** | SW | Internal OK |
| **Encoder 2** | Volume A | **GPIO 34** | CLK | **EXTERNAL 10kΩ** |
| (Vol) | Volume B | **GPIO 35** | DT | **EXTERNAL 10kΩ** |
| | Back / Exit | **GPIO 39** | SW | **EXTERNAL 10kΩ** |
| **Footswitch** | Previous | **GPIO 32** | - | Internal OK |
| **Footswitch** | Play / Stop | **GPIO 33** | - | Internal OK |
| **Footswitch** | Next | **GPIO 27** | - | Internal OK |

---

## ⚡ Power Supply Wiring

To avoid digital noise in the audio signal (a common issue with ESP32 audio):

1.  **Input:** 9V DC (Center Negative - Standard Guitar Pedal Power).
2.  **Regulation:** Use the **MP1584EN** or **LM2596** Buck Converter to step down 9V -> 5V.
    * *Important:* If using LM2596, connect it to 9V and adjust the trimpot screw until the output reads **5.0V** on a multimeter **BEFORE** connecting it to the ESP32.
3.  **Isolation (Optional but Recommended):**
    * Power the ESP32 via the 5V/VIN pin.
    * Power the PCM5102 DAC directly from the 5V source (before it enters the ESP32) or use a separate LDO (Low Drop-Out regulator) like an AMS1117-3.3V dedicated to the DAC analog side if your module supports it.
    * Add a large capacitor (e.g., 470uF) on the 5V rail near the audio components.

## 📂 SD Card Structure

Format your SD card as **FAT32**. Create folders for your banks at the root.

```text
/ (Root)
├── Ambient 1/
│   ├── C.mp3
│   ├── C#.mp3
│   └── ... (up to B.mp3)
├── Worship Pads/
│   ├── C.mp3
│   └── ...
└── Organ Drones/
    └── ...
```
> Note: The system creates a virtual list of these folders on boot.