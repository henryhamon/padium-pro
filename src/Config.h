#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions ---
// I2S DAC
#define I2S_BCLK 26
#define I2S_LRCK 25
#define I2S_DOUT 22

// SD Card (VSPI)
#define SD_CS 5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18

// Controls
// WARNING: GPIOs 34, 35, 39 are INPUT-ONLY and have NO internal pull-ups.
// You MUST add external 10k pull-up resistors to 3V3 for these pins to work.
#define PIN_VOL_ENC_A 34
#define PIN_VOL_ENC_B 35
#define PIN_VOL_ENC_BTN 39
#define PIN_PREV 32
#define PIN_PLAY 33
#define PIN_NEXT 27
#define PIN_ENC_A 16
#define PIN_ENC_B 17
#define PIN_ENC_BTN 21

// Display (HSPI)
#define PIN_TFT_BL 4 // Example GPIO for Backlight control
// #define TFT_SCLK 14
// #define TFT_CS   15
// #define TFT_DC   2
// #define TFT_RST  4

// --- Constants ---
#define UI_SCREEN_WIDTH 240
#define UI_SCREEN_HEIGHT 240

// Colors - DEPRECATED (Moved to Dynamic Theme in UI_Logic)
// Legacy colors removed to prevent usage.
// Use UI_Controller::applyTheme and members or TFT_Xx constants.

#endif
