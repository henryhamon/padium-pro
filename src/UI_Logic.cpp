#include "UI_Logic.h"
#include "Config.h"

UI_Controller::UI_Controller() : tft() { sprite = new TFT_eSprite(&tft); }

void UI_Controller::init() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK); // Initial clear

  // Create a 240x240 sprite for full screen update
  sprite->createSprite(240, 240);

  // Default Theme (Dark)
  applyTheme(true);
}

void UI_Controller::applyTheme(bool isDark) {
  if (isDark) {
    colorBg = TFT_BLACK;
    colorText = TFT_WHITE;
    colorAccent = TFT_SKYBLUE;
    colorHill = 0x10A2; // Dark Grey-ish
    colorHighlight = TFT_GREEN;
  } else {
    colorBg = TFT_WHITE;
    colorText = TFT_BLACK;
    colorAccent = TFT_BLUE;
    colorHill = 0xC618;             // Light Grey
    colorHighlight = TFT_DARKGREEN; // Darker green for contrast on white
  }
}

void UI_Controller::drawConvexBackground() {
  sprite->fillSprite(colorBg);

  // Draw the "Hill" - a gentle arch at the bottom
  sprite->fillCircle(120, 330, 160, colorHill);
}

void UI_Controller::drawText(const char *text, int x, int y, uint8_t font,
                             uint16_t color, uint8_t datum) {
  sprite->setTextColor(color, colorBg);
  sprite->setTextDatum(datum);
  sprite->setTextFont(font);
  sprite->drawString(text, x, y);
}

// PERFORMANCE VIEW
void UI_Controller::drawPerformance(const char *currentKey, const char *nextKey,
                                    const char *presetName, int volume,
                                    int fadeTimeMs, bool useCrossfade) {
  drawConvexBackground();

  // Current Key (Large, center top)
  sprite->setTextSize(3);
  sprite->setTextColor(colorText);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(currentKey, 120, 80, 4);
  sprite->setTextSize(1); // Reset

  // Preset Name (Small, above Key)
  sprite->setTextColor(colorAccent);
  sprite->drawString(presetName, 120, 40, 2);

  // Next Key (Bottom area)
  char nextBuffer[32];
  sprintf(nextBuffer, "NEXT: %s", nextKey);
  sprite->setTextColor(colorText);
  sprite->drawString(nextBuffer, 120, 170, 2);

  // Params (Volume)
  char volBuffer[16];
  sprintf(volBuffer, "VOL: %d", volume);
  sprite->setTextColor(colorText);
  sprite->drawString(volBuffer, 60, 200, 2);

  // Fade Time (Small info)
  char fadeBuffer[16];
  sprintf(fadeBuffer, "%ds", fadeTimeMs / 1000);
  sprite->drawString(fadeBuffer, 180, 200, 2);

  // Transition Mode icon/text
  const char *transText = useCrossfade ? "XFADE" : "CUT";
  sprite->drawString(transText, 120, 215, 2);

  sprite->pushSprite(0, 0);
}

// MENU VIEW
void UI_Controller::drawMenu(int selectedIndex, bool isEditing, int fadeTimeMs,
                             bool useCrossfade, bool isDark, int brightness) {
  sprite->fillSprite(colorBg);

  // Header
  sprite->setTextColor(colorAccent);
  sprite->setTextSize(1);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString("- SETTINGS -", 120, 30, 2);

  // List Items
  const int startY = 70;
  const int gapY = 30;

  const char *labels[] = {"Fade Time", "Trans.", "Theme", "Bright", "Exit"};

  for (int i = 0; i < MENU_COUNT; i++) {
    int y = startY + (i * gapY);

    // Color Logic
    uint16_t itemColor = colorText;
    if (i == selectedIndex) {
      itemColor = colorHighlight; // Green if selected
      if (isEditing)
        itemColor = TFT_RED; // Red if editing value
    }

    sprite->setTextColor(itemColor);
    sprite->setTextDatum(MR_DATUM); // Align Right for Label
    sprite->drawString(labels[i], 110, y, 2);

    // Value Draw
    char valBuffer[32];
    switch (i) {
    case MENU_FADE_TIME:
      sprintf(valBuffer, "%ds", fadeTimeMs / 1000);
      break;
    case MENU_TRANSITION:
      sprintf(valBuffer, "%s", useCrossfade ? "XFade" : "Cut");
      break;
    case MENU_THEME:
      sprintf(valBuffer, "%s", isDark ? "Dark" : "Light");
      break;
    case MENU_BRIGHTNESS:
      sprintf(valBuffer, "%d%%", (brightness * 100) / 255);
      break;
    case MENU_EXIT:
      sprintf(valBuffer, "Return");
      break; // Polished Text: Return
    }

    sprite->setTextDatum(ML_DATUM); // Align Left for Value
    sprite->drawString(valBuffer, 130, y, 2);
  }

  sprite->pushSprite(0, 0);
}

void UI_Controller::showSplashScreen() {
  sprite->fillSprite(TFT_BLACK); // Always Black splash

  sprite->setTextColor(TFT_WHITE);
  sprite->setTextDatum(MC_DATUM);

  // PADIUM
  sprite->setTextSize(3); // Large
  sprite->drawString("PADIUM", 120, 100, 4);

  // PRO
  sprite->setTextSize(2); // Med
  // Use Hardcoded Green for Brand
  sprite->setTextColor(TFT_GREEN);
  sprite->drawString("PRO", 120, 140, 4);

  // System Check
  sprite->setTextSize(1);
  sprite->setTextColor(TFT_SILVER);
  sprite->drawString("System Check...", 120, 200, 2);

  sprite->pushSprite(0, 0);
}

void UI_Controller::showErrorScreen(const char *errorMessage) {
  // Hardcoded Alert Colors
  sprite->fillSprite(TFT_RED);

  sprite->setTextColor(TFT_YELLOW);
  sprite->setTextDatum(MC_DATUM);
  sprite->setTextSize(2);

  sprite->drawString(errorMessage, 120, 120, 2);

  sprite->pushSprite(0, 0);
}
