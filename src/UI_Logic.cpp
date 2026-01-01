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
  snprintf(nextBuffer, sizeof(nextBuffer), "NEXT: %s", nextKey);
  sprite->setTextColor(colorText);
  sprite->drawString(nextBuffer, 120, 170, 2);

  // Params (Volume)
  char volBuffer[16];
  snprintf(volBuffer, sizeof(volBuffer), "VOL: %d", volume);
  sprite->setTextColor(colorText);
  sprite->drawString(volBuffer, 60, 200, 2);

  // Fade Time (Small info)
  char fadeBuffer[16];
  snprintf(fadeBuffer, sizeof(fadeBuffer), "%ds", fadeTimeMs / 1000);
  sprite->drawString(fadeBuffer, 180, 200, 2);

  // Transition Mode icon/text
  const char *transText = useCrossfade ? "XFADE" : "CUT";
  sprite->drawString(transText, 120, 215, 2);

  sprite->pushSprite(0, 0);
}

// Menu Labels (Global or Static)
const char *MENU_LABELS[MENU_COUNT] = {"Fade Time", "Trans.",    "Theme",
                                       "Bright",    "Wi-Fi Mgr", "Return"};

// MENU VIEW
void UI_Controller::drawMenu(int selectedIndex, bool isEditing, int fadeTimeMs,
                             bool useCrossfade, bool isDark, int brightness) {
  sprite->fillSprite(colorBg);

  // Header
  sprite->setTextColor(colorAccent);
  sprite->setTextSize(1);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString("- SETTINGS -", 120, 25, 2);

  // List Items
  const int startY = 60;
  const int gapY = 28;

  for (int i = 0; i < MENU_COUNT; i++) {
    int y = startY + (i * gapY);

    // Color Logic
    uint16_t itemColor = colorText;
    if (i == selectedIndex) {
      itemColor = colorHighlight;
      if (isEditing)
        itemColor = TFT_RED;
    }

    sprite->setTextColor(itemColor);
    sprite->setTextDatum(MR_DATUM);
    sprite->drawString(MENU_LABELS[i], 110, y, 2);

    // Value Draw
    char valBuffer[32] = "";
    switch (i) {
    case MENU_FADE_TIME:
      snprintf(valBuffer, sizeof(valBuffer), "%ds", fadeTimeMs / 1000);
      break;
    case MENU_TRANSITION:
      snprintf(valBuffer, sizeof(valBuffer), "%s",
               useCrossfade ? "XFade" : "Cut");
      break;
    case MENU_THEME:
      snprintf(valBuffer, sizeof(valBuffer), "%s", isDark ? "Dark" : "Light");
      break;
    case MENU_BRIGHTNESS:
      snprintf(valBuffer, sizeof(valBuffer), "%d%%", (brightness * 100) / 255);
      break;
    // Wi-Fi and Return have dynamic "action" text or can use fixed text
    case MENU_WIFI:
      snprintf(valBuffer, sizeof(valBuffer), "Start");
      break;
    case MENU_EXIT:
      snprintf(valBuffer, sizeof(valBuffer), "Return");
      break;
    }

    sprite->setTextDatum(ML_DATUM);
    sprite->drawString(valBuffer, 130, y, 2);
  }

  sprite->pushSprite(0, 0);
}

// WIFI SCREEN
void UI_Controller::drawWifiScreen(const char *ssid, const char *ip) {
  sprite->fillSprite(TFT_BLACK); // Always Dark for tech mode

  sprite->setTextDatum(MC_DATUM);

  // TITLE
  sprite->setTextColor(TFT_GREEN);
  sprite->setTextSize(1);
  sprite->drawString("WI-FI MANAGER", 120, 50, 4); // Big Font
  sprite->drawString("ACTIVE", 120, 80, 2);

  // INFO
  sprite->setTextColor(TFT_WHITE);
  sprite->setTextSize(1);

  char ssidBuf[64];
  snprintf(ssidBuf, sizeof(ssidBuf), "SSID: %s", ssid);
  sprite->drawString(ssidBuf, 120, 130, 2);

  char ipBuf[64];
  snprintf(ipBuf, sizeof(ipBuf), "IP: %s", ip);
  sprite->drawString(ipBuf, 120, 155, 2);

  // EXIT INSTRUCTION
  sprite->setTextColor(TFT_RED);
  sprite->drawString("PRESS VOL BUTTON", 120, 210, 2);
  sprite->drawString("TO EXIT", 120, 230, 2);

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
