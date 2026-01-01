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
#define PIN_POT 34 // ADC1
#define PIN_PREV 32
#define PIN_PLAY 33
#define PIN_NEXT 27
#define PIN_ENC_A 16
#define PIN_ENC_B 17
#define PIN_ENC_BTN 21

// Display (HSPI) - Defined in platformio.ini but good to have reference or
// specific control pins if needed #define TFT_MISO 12 #define TFT_MOSI 13
// #define TFT_SCLK 14
// #define TFT_CS   15
// #define TFT_DC   2
// #define TFT_RST  4

// --- Constants ---
#define UI_SCREEN_WIDTH 240
#define UI_SCREEN_HEIGHT 240

// Colors
#define C_BG TFT_BLACK
#define C_ACCENT TFT_SKYBLUE
#define C_TEXT TFT_WHITE
#define C_HILL 0x10A2
#define C_HIGHLIGHT TFT_GREEN

#endif
