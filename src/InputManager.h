#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Config.h"
#include <Arduino.h>

class InputManager {
public:
  void init();
  void update();

  // Encoders
  int getVolumeDelta(); // Returns +1, -1, or 0
  int getNavDelta();    // Returns +1, -1, or 0

  // Buttons (Events)
  bool wasVolBtnPressed(); // Click event
  bool wasNavBtnPressed(); // Click event

  bool wasNextPressed();
  bool wasPrevPressed();
  bool wasPlayPressed();

  // Buttons (State)
  bool isPlayHeld(); // For Panic
  bool isNextHeld(); // For Fast Scroll
  bool isPrevHeld(); // For Fast Scroll

private:
  // Internal State
  int lastVolClk = HIGH;
  int lastNavClk = HIGH;

  // Debounce & Hold Timers
  unsigned long lastVolBtnTime = 0;
  unsigned long lastNavBtnTime = 0;

  unsigned long btnNextHoldTime = 0;
  unsigned long btnPrevHoldTime = 0;
  unsigned long btnPlayHoldTime = 0;
  unsigned long lastRepeatTime = 0;

  // State Flags for Hold
  bool playHeldState = false;
  bool nextHeldRepeat = false;
  bool prevHeldRepeat = false;

  // Buffered Events (cleared after read)
  int volDelta = 0;
  int navDelta = 0;
  bool volBtnPressed = false;
  bool navBtnPressed = false;
  bool nextPressed = false;
  bool prevPressed = false;
  bool playPressed = false;

  // Constants
  const int DEBOUNCE_MS = 50;
  const int HOLD_DELAY_MS = 400;
  const int REPEAT_RATE_MS = 100;
  const int PANIC_DELAY_MS = 1000;
};

#endif
