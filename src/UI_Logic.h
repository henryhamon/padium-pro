#ifndef UI_LOGIC_H
#define UI_LOGIC_H

#include <SPI.h>
#include <TFT_eSPI.h>

enum EditMode {
  MODE_PRESET,
  MODE_FADE_TIME,
  MODE_TRANSITION
  // We keep this specific enum for main.cpp logic legacy or internal UI usage?
  // Actually, plan says "Edit Mode" is now distinct from "Menu View".
  // Let's redefine EditMode or keep it for compatibility if implementation uses
  // it. Plan: "Encoder Click enters "Edit Mode" (value turns red/green)..."
};

// Menu Options
enum MenuOption {
  MENU_FADE_TIME,
  MENU_TRANSITION,
  MENU_THEME,
  MENU_BRIGHTNESS,
  MENU_EXIT,
  MENU_COUNT // Total items
};

class UI_Controller {
public:
  UI_Controller();
  void init();

  // New Theme Engine
  void applyTheme(bool isDark);

  // Render Methods
  void drawPerformance(const char *currentKey, const char *nextKey,
                       const char *presetName, int volume, int fadeTimeMs,
                       bool useCrossfade);

  void drawMenu(int selectedIndex, bool isEditing, int fadeTimeMs,
                bool useCrossfade, bool isDark, int brightness);

  void showSplashScreen();
  void showErrorScreen(const char *errorMessage);

  // Legacy update method kept/modified if needed or deprecated
  // void update(...);

private:
  TFT_eSprite *sprite;
  TFT_eSPI tft;
  void drawConvexBackground();
  void drawText(const char *text, int x, int y, uint8_t font, uint16_t color,
                uint8_t datum);

  // Dynamic Theme Colors
  uint16_t colorBg;
  uint16_t colorText;
  uint16_t colorAccent;
  uint16_t colorHill;
  uint16_t colorHighlight;
};

#endif
