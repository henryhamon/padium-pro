#include "UI_Logic.h"
#include "Config.h"

UI_Controller::UI_Controller() : tft() { sprite = new TFT_eSprite(&tft); }

void UI_Controller::init() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(C_BG);

  // Create a 240x240 sprite for full screen update
  sprite->createSprite(240, 240);
}

void UI_Controller::drawConvexBackground() {
  sprite->fillSprite(C_BG);

  // Draw the "Hill" - a gentle arch at the bottom
  sprite->fillCircle(120, 330, 160, C_HILL);
}

void UI_Controller::drawText(const char *text, int x, int y, uint8_t font,
                             uint16_t color, uint8_t datum) {
  sprite->setTextColor(color, C_BG); // BG color is ignored in transparent
                                     // sprite mode mostly but good practice
  sprite->setTextDatum(datum);
  sprite->setTextFont(font); // internal fonts for now

  // For bespoke fonts, user would load them separately.
  // Using internal font 4 (26px) and 2 (16px) for simplicity

  sprite->drawString(text, x, y);
}

void UI_Controller::update(const char *currentKey, const char *nextKey,
                           const char *presetName, int volume, int fadeTimeMs,
                           EditMode mode, bool useCrossfade) {
  drawConvexBackground();

  // Current Key (Large, center top)
  sprite->setTextSize(3); // Scale up font 4
  sprite->setTextColor(C_TEXT);
  sprite->setTextDatum(MC_DATUM);
  sprite->drawString(currentKey, 120, 80, 4);
  sprite->setTextSize(1); // Reset

  // Preset Name (Small, above Key)
  // Highlight if active
  uint16_t presetColor = (mode == MODE_PRESET) ? C_HIGHLIGHT : C_ACCENT;
  sprite->setTextColor(presetColor);
  sprite->drawString(presetName, 120, 40, 2);

  // Next Key (Bottom area, inside the hill)
  char nextBuffer[32];
  sprintf(nextBuffer, "NEXT: %s", nextKey);
  sprite->setTextColor(TFT_SILVER);
  sprite->drawString(nextBuffer, 120, 170, 2);

  // Params (Volume) - Volume is not part of the modal edit in this requirement,
  // usually just Pot
  char volBuffer[16];
  sprintf(volBuffer, "VOL: %d", volume);
  sprite->setTextColor(TFT_SILVER); // Always silver or white
  sprite->drawString(volBuffer, 60, 200, 2);

  // Fade Time
  char fadeBuffer[16];
  sprintf(fadeBuffer, "FADE: %ds", fadeTimeMs / 1000);
  uint16_t fadeColor = (mode == MODE_FADE_TIME) ? C_HIGHLIGHT : TFT_SILVER;
  sprite->setTextColor(fadeColor);
  sprite->drawString(fadeBuffer, 180, 200, 2);

  // Transition Mode
  const char *transText = useCrossfade ? "XFADE" : "CUT";
  uint16_t transColor = (mode == MODE_TRANSITION) ? C_HIGHLIGHT : TFT_SILVER;
  sprite->setTextColor(transColor);
  sprite->drawString(transText, 120, 215,
                     2); // Positioned between Vol and Fade, slightly lower

  // Push sprite to screen
  sprite->pushSprite(0, 0);
}
