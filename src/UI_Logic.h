#ifndef UI_LOGIC_H
#define UI_LOGIC_H

#include <TFT_eSPI.h>

enum EditMode { MODE_PRESET, MODE_FADE_TIME, MODE_TRANSITION };

class UI_Controller {
public:
  UI_Controller();
  void init();
  void update(const char *currentKey, const char *nextKey,
              const char *presetName, int volume, int fadeTimeMs, EditMode mode,
              bool useCrossfade);

private:
  TFT_eSPI tft;
  TFT_eSprite *sprite;

  void drawConvexBackground();
  void drawText(const char *text, int x, int y, uint8_t font, uint16_t color,
                uint8_t datum);
};

#endif
