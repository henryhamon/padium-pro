#include "InputManager.h"

void InputManager::init() {
  // Nav Encoder
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);

  // Vol Encoder
  pinMode(PIN_VOL_ENC_A, INPUT);
  pinMode(PIN_VOL_ENC_B, INPUT);
  pinMode(PIN_VOL_ENC_BTN, INPUT);

  // Footswitches
  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);
}

void InputManager::update() {
  unsigned long now = millis();

  // 1. Volume Encoder
  int currentVolClk = digitalRead(PIN_VOL_ENC_A);
  if (currentVolClk != lastVolClk && currentVolClk == LOW) {
    int dt = digitalRead(PIN_VOL_ENC_B);
    volDelta += (dt != currentVolClk) ? 1 : -1;
  }
  lastVolClk = currentVolClk;

  // 2. Nav Encoder
  int currentNavClk = digitalRead(PIN_ENC_A);
  if (currentNavClk != lastNavClk && currentNavClk == LOW) {
    int dt = digitalRead(PIN_ENC_B);
    navDelta += (dt != currentNavClk) ? 1 : -1;
  }
  lastNavClk = currentNavClk;

  // 3. Volume Button (Back)
  static int lastVolBtnState = HIGH;
  int volBtnState = digitalRead(PIN_VOL_ENC_BTN);
  if (lastVolBtnState == HIGH && volBtnState == LOW) {
    if (now - lastVolBtnTime > DEBOUNCE_MS) {
      volBtnPressed = true;
      lastVolBtnTime = now;
    }
  }
  lastVolBtnState = volBtnState;

  // 4. Nav Button (Select)
  static int lastNavBtnState = HIGH;
  int navBtnState = digitalRead(PIN_ENC_BTN);
  if (lastNavBtnState == HIGH && navBtnState == LOW) {
    if (now - lastNavBtnTime > DEBOUNCE_MS) {
      navBtnPressed = true;
      lastNavBtnTime = now;
    }
  }
  lastNavBtnState = navBtnState;

  // 5. NEXT Button (Hold & Press)
  if (digitalRead(PIN_NEXT) == LOW) {
    if (btnNextHoldTime == 0) {
      btnNextHoldTime = now;
    } else {
      // Holding
      if (now - btnNextHoldTime > HOLD_DELAY_MS) {
        // Repeat Logic
        if (now - lastRepeatTime > REPEAT_RATE_MS) {
          nextPressed = true;    // Trigger Event repeatedly
          nextHeldRepeat = true; // Internal flag if needed
          lastRepeatTime = now;
        }
      }
    }
  } else {
    if (btnNextHoldTime > 0) {
      // Released
      if (now - btnNextHoldTime < HOLD_DELAY_MS) {
        nextPressed = true; // Short Press
      }
      btnNextHoldTime = 0;
      nextHeldRepeat = false;
    }
  }

  // 6. PREV Button
  if (digitalRead(PIN_PREV) == LOW) {
    if (btnPrevHoldTime == 0) {
      btnPrevHoldTime = now;
    } else {
      if (now - btnPrevHoldTime > HOLD_DELAY_MS) {
        if (now - lastRepeatTime > REPEAT_RATE_MS) {
          prevPressed = true;
          prevHeldRepeat = true;
          lastRepeatTime = now;
        }
      }
    }
  } else {
    if (btnPrevHoldTime > 0) {
      if (now - btnPrevHoldTime < HOLD_DELAY_MS) {
        prevPressed = true;
      }
      btnPrevHoldTime = 0;
      prevHeldRepeat = false;
    }
  }

  // 7. PLAY Button (Panic)
  if (digitalRead(PIN_PLAY) == LOW) {
    if (btnPlayHoldTime == 0) {
      btnPlayHoldTime = now;
    } else {
      if (now - btnPlayHoldTime > PANIC_DELAY_MS) {
        playHeldState = true;
      }
    }
  } else {
    if (btnPlayHoldTime > 0 && !playHeldState) {
      playPressed = true;
    }
    btnPlayHoldTime = 0;
    playHeldState = false;
  }
}

int InputManager::getVolumeDelta() {
  int d = volDelta;
  volDelta = 0;
  return d;
}

int InputManager::getNavDelta() {
  int d = navDelta;
  navDelta = 0;
  return d;
}

bool InputManager::wasVolBtnPressed() {
  bool b = volBtnPressed;
  volBtnPressed = false;
  return b;
}

bool InputManager::wasNavBtnPressed() {
  bool b = navBtnPressed;
  navBtnPressed = false;
  return b;
}

bool InputManager::wasNextPressed() {
  bool b = nextPressed;
  nextPressed = false;
  return b;
}

bool InputManager::wasPrevPressed() {
  bool b = prevPressed;
  prevPressed = false;
  return b;
}

bool InputManager::wasPlayPressed() {
  bool b = playPressed;
  playPressed = false;
  return b;
}

bool InputManager::isPlayHeld() { return playHeldState; }

bool InputManager::isNextHeld() { return nextHeldRepeat; }

bool InputManager::isPrevHeld() { return prevHeldRepeat; }
