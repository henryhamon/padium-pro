#include "AudioTask.h"
#include "Config.h"
#include "UI_Logic.h"
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>

// --- Globals ---
UI_Controller ui;
Preferences prefs;

// State Variables
int currentPresetIndex = 0;
const char *presets[] = {"Ambient 1", "Warm Pad", "Shimmer", "Drone C"};
const int numPresets = 4;

// 1. Chromatic Scale (12 keys)
const char *keys[] = {"C",  "C#", "D",  "D#", "E",  "F",
                      "F#", "G",  "G#", "A",  "A#", "B"};
const int numKeys = 12;

int currentKeyIndex = 0; // What is playing
int nextKeyIndex = 0;    // What is queued

int volume = 21;
bool isPlayingState = false;

// Settings & Persistence
int fadeTimeMs = 1000;
bool useCrossfade = true;
int screenBrightness = 255;
bool isDarkMode = true;

// UI State Machine
enum UIState { VIEW_PERFORMANCE, VIEW_MENU };
UIState uiState = VIEW_PERFORMANCE;

// Menu Logic
int menuIndex = 0;
bool isMenuEditing = false;

// Encoder State
int lastClk = HIGH;

// Button Debounce & Hold Logic
unsigned long lastDebounceTime = 0;
unsigned long btnNextHoldTime = 0;
bool btnNextHeld = false;
unsigned long btnPrevHoldTime = 0;
bool btnPrevHeld = false;
unsigned long btnPlayHoldTime = 0; // For Panic
const int HOLD_DELAY = 400;        // Time before repeat starts
const int REPEAT_RATE = 100;       // Repeat every 100ms
const int PANIC_DELAY = 1000;      // 1s hold for Panic

// Screensaver
unsigned long lastInteractionTime = 0;
bool isDimmed = false;

// --- Helper Functions ---

void resetScreensaver() {
  lastInteractionTime = millis();
  if (isDimmed) {
    isDimmed = false;
#ifdef PIN_TFT_BL
    ledcWrite(0, screenBrightness);
#endif
  }
}

void updateBrightness() {
#ifdef PIN_TFT_BL
  ledcWrite(0, screenBrightness);
#endif
}

void loadSettings() {
  prefs.begin("padium", true); // Read only
  volume = prefs.getInt("vol", 21);
  fadeTimeMs = prefs.getInt("fade", 1000);
  useCrossfade = prefs.getBool("xfade", true);
  currentPresetIndex = prefs.getInt("preset", 0);
  screenBrightness = prefs.getInt("bright", 255);
  isDarkMode = prefs.getBool("theme", true);
  prefs.end();
}

void saveSettings() {
  prefs.begin("padium", false); // Read/Write
  prefs.putInt("vol", volume);
  prefs.putInt("fade", fadeTimeMs);
  prefs.putBool("xfade", useCrossfade);
  prefs.putInt("preset", currentPresetIndex);
  prefs.putInt("bright", screenBrightness);
  prefs.putBool("theme", isDarkMode);
  prefs.end();
}

void updateUI() {
  if (uiState == VIEW_PERFORMANCE) {
    ui.drawPerformance(keys[currentKeyIndex], keys[nextKeyIndex],
                       presets[currentPresetIndex], volume, fadeTimeMs,
                       useCrossfade);
  } else {
    ui.drawMenu(menuIndex, isMenuEditing, fadeTimeMs, useCrossfade, isDarkMode,
                screenBrightness);
  }
}

void handleMenuScroll(int direction) {
  if (!isMenuEditing) {
    // Scroll List
    menuIndex += direction;
    if (menuIndex < 0)
      menuIndex = MENU_COUNT - 1;
    if (menuIndex >= MENU_COUNT)
      menuIndex = 0;
  } else {
    // Edit Value
    MenuOption opt = (MenuOption)menuIndex;
    switch (opt) {
    case MENU_FADE_TIME:
      fadeTimeMs += (direction * 500);
      if (fadeTimeMs < 0)
        fadeTimeMs = 0;
      if (fadeTimeMs > 10000)
        fadeTimeMs = 10000;
      break;
    case MENU_TRANSITION:
      if (direction != 0)
        useCrossfade = !useCrossfade;
      break;
    case MENU_THEME:
      if (direction != 0) {
        isDarkMode = !isDarkMode;
        ui.applyTheme(isDarkMode); // Apply immediately
      }
      break;
    case MENU_BRIGHTNESS:
      screenBrightness += (direction * 25);
      if (screenBrightness < 10)
        screenBrightness = 10;
      if (screenBrightness > 255)
        screenBrightness = 255;
      updateBrightness();
      break;
    default:
      break;
    }
  }
  updateUI();
}

void handleMenuClick() {
  MenuOption opt = (MenuOption)menuIndex;
  if (opt == MENU_EXIT) {
    uiState = VIEW_PERFORMANCE;
    saveSettings();
    updateUI();
    return;
  }

  // Toggle Edit Mode
  isMenuEditing = !isMenuEditing;

  // If we just finished editing, save settings
  if (!isMenuEditing) {
    saveSettings();
  }
  updateUI();
}

void scanControls() {
  // 1. Volume Encoder (Volume + BACK)
  static int lastVolClk = HIGH;
  int currentVolClk = digitalRead(PIN_VOL_ENC_A);
  if (currentVolClk != lastVolClk && currentVolClk == LOW) { // Falling Edge
    resetScreensaver();
    int dtValue = digitalRead(PIN_VOL_ENC_B);
    int direction = (dtValue != currentVolClk) ? 1 : -1;

    int newVol = volume + direction;
    if (newVol < 0)
      newVol = 0;
    if (newVol > 21)
      newVol = 21;

    if (newVol != volume) {
      volume = newVol;
      AudioCommand cmd;
      cmd.type = CMD_SET_VOLUME;
      cmd.value = volume;
      xQueueSend(audioQueue, &cmd, 0);

      if (uiState == VIEW_PERFORMANCE) {
        updateUI();
      }
    }
    // saveSettings(); // Deferred save
  }
  lastVolClk = currentVolClk;

  // Volume Button (BACK / EXIT)
  if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
    resetScreensaver();
    delay(50);
    if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
      if (uiState == VIEW_MENU) {
        uiState = VIEW_PERFORMANCE;
        isMenuEditing = false;
        // Save on exit
        saveSettings();
        updateUI();
      }
    }
  }

  // 2. Navigation Encoder (Preset / Menu)
  int currentClk = digitalRead(PIN_ENC_A);
  if (currentClk != lastClk && currentClk == LOW) { // Falling Edge
    resetScreensaver();
    int dtValue = digitalRead(PIN_ENC_B);
    int direction = (dtValue != currentClk) ? 1 : -1;

    if (uiState == VIEW_PERFORMANCE) {
      // Change Preset
      currentPresetIndex += direction;
      if (currentPresetIndex < 0)
        currentPresetIndex = numPresets - 1;
      if (currentPresetIndex >= numPresets)
        currentPresetIndex = 0;
      updateUI();
    } else {
      // Menu Navigate
      handleMenuScroll(direction);
    }
  }
  lastClk = currentClk;

  // Navigation Button (Enter Menu / Select)
  static int lastBtnState = HIGH;
  int btnState = digitalRead(PIN_ENC_BTN);
  if (lastBtnState == HIGH && btnState == LOW) { // Press
    resetScreensaver();
    delay(50);
    if (digitalRead(PIN_ENC_BTN) == LOW) {
      if (uiState == VIEW_PERFORMANCE) {
        uiState = VIEW_MENU;
        menuIndex = 0;
        isMenuEditing = false;
        updateUI();
      } else {
        handleMenuClick();
      }
    }
  }
  lastBtnState = btnState;

  // 3. Navigation Buttons (Only in Performance View)
  if (uiState == VIEW_PERFORMANCE) {
    if (digitalRead(PIN_NEXT) == LOW) {
      resetScreensaver();
      if (btnNextHoldTime == 0) { // First press
        btnNextHoldTime = millis();
        btnNextHeld = false;
        nextKeyIndex = (nextKeyIndex + 1) % numKeys;
        updateUI();
      } else {
        if (millis() - btnNextHoldTime > HOLD_DELAY) {
          if (millis() - lastDebounceTime > REPEAT_RATE) {
            nextKeyIndex = (nextKeyIndex + 2) % numKeys;
            updateUI();
            lastDebounceTime = millis();
          }
        }
      }
    } else {
      btnNextHoldTime = 0;
    }

    if (digitalRead(PIN_PREV) == LOW) {
      resetScreensaver();
      if (btnPrevHoldTime == 0) {
        btnPrevHoldTime = millis();
        nextKeyIndex = (nextKeyIndex - 1 + numKeys) % numKeys;
        updateUI();
      } else {
        if (millis() - btnPrevHoldTime > HOLD_DELAY) {
          if (millis() - lastDebounceTime > REPEAT_RATE) {
            nextKeyIndex = (nextKeyIndex - 2 + numKeys) % numKeys;
            updateUI();
            lastDebounceTime = millis();
          }
        }
      }
    } else {
      btnPrevHoldTime = 0;
    }

    // 4. Play Button (Panic & Soft Stop)
    int playState = digitalRead(PIN_PLAY);
    if (playState == LOW) {
      resetScreensaver();
      if (btnPlayHoldTime == 0) {
        btnPlayHoldTime = millis(); // Start Hold
      } else {
        // Check for Panic Hold
        // If held > 1000ms, Stop Immediately
        if (!isDimmed && (millis() - btnPlayHoldTime > PANIC_DELAY)) {
          // PANIC MODE
          AudioCommand cmd;
          // Force Silence first
          cmd.type = CMD_SET_VOLUME;
          cmd.value = 0;
          xQueueSend(audioQueue, &cmd, 0);
          // Then Stop
          cmd.type = CMD_STOP;
          xQueueSend(audioQueue, &cmd, 0);
          isPlayingState = false;
          updateUI(); // AudioTask will set state to IDLE

          // Block until release to prevent re-trigger
          while (digitalRead(PIN_PLAY) == LOW) {
            delay(10);
          }
          btnPlayHoldTime = 0;
          return; // Exit
        }
      }
    } else {
      // Release - Trigger Action if Short Press
      if (btnPlayHoldTime > 0) {
        long holdDuration = millis() - btnPlayHoldTime;
        if (holdDuration < PANIC_DELAY) {
          // Short Press = Trigger / Soft Stop
          if (!isPlayingState) {
            // START
            currentKeyIndex = nextKeyIndex;
            AudioCommand cmd;
            cmd.type = CMD_PLAY;

            char safeKey[8];
            strcpy(safeKey, keys[currentKeyIndex]);
            if (safeKey[1] == '#') {
              safeKey[1] = 's';
              safeKey[2] = '\0';
            }

            sprintf(cmd.filename, "/%s/%s.mp3", presets[currentPresetIndex],
                    safeKey);
            xQueueSend(audioQueue, &cmd, 0);
            isPlayingState = true;
          } else {
            // ALREADY PLAYING / TRANSITION
            if (nextKeyIndex != currentKeyIndex) {
              currentKeyIndex = nextKeyIndex;
              AudioCommand cmd;
              if (useCrossfade) {
                cmd.type = CMD_CROSSFADE;
                cmd.value = fadeTimeMs;
              } else {
                cmd.type = CMD_PLAY;
              }
              char safeKey[8];
              strcpy(safeKey, keys[currentKeyIndex]);
              if (safeKey[1] == '#') {
                safeKey[1] = 's';
                safeKey[2] = '\0';
              }
              sprintf(cmd.filename, "/%s/%s.mp3", presets[currentPresetIndex],
                      safeKey);
              xQueueSend(audioQueue, &cmd, 0);
            } else {
              // STOP (Soft)
              AudioCommand cmd;
              cmd.type = CMD_STOP;
              xQueueSend(audioQueue, &cmd, 0);
              isPlayingState = false;
            }
          }
          updateUI();
        }
        btnPlayHoldTime = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Load Settings
  loadSettings();

  // 1. Setup Controls
  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);

  // Volume Encoder (Input Only - Requires External Pullups)
  pinMode(PIN_VOL_ENC_A, INPUT);
  pinMode(PIN_VOL_ENC_B, INPUT);
  pinMode(PIN_VOL_ENC_BTN, INPUT);

// Backlight PWM
#ifdef PIN_TFT_BL
  pinMode(PIN_TFT_BL, OUTPUT);
  ledcSetup(0, 5000, 8); // Channel 0, 5KHz, 8-bit
  ledcAttachPin(PIN_TFT_BL, 0);
  updateBrightness();
#endif

  // Reset Idle Timer
  resetScreensaver();

  // 2. Initialize UI
  ui.init();
  ui.applyTheme(isDarkMode); // Apply loaded theme

  // 3. SPLASH
  ui.showSplashScreen();
  delay(1000);

  // 4. SD CHECK
  SPIClass tempSPI(VSPI);
  tempSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, tempSPI)) {
    ui.showErrorScreen("NO SD CARD");
    while (true) {
      delay(100);
    }
  }

  // 5. FILE CHECK
  char checkPath[32];
  sprintf(checkPath, "/%s", presets[0]);
  if (!SD.exists(checkPath)) {
    ui.showErrorScreen("MISSING FILES");
    while (true) {
      delay(100);
    }
  }
  SD.end();

  // 6. Audio Task
  audioQueue = xQueueCreate(10, sizeof(AudioCommand));
  sdCardMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096 * 4, NULL, 2, NULL, 0);

  // 7. Start UI
  updateUI();
}

void loop() {
  scanControls();

  // Screensaver Logic
  if (!isDimmed && (millis() - lastInteractionTime > 30000)) {
    isDimmed = true;
#ifdef PIN_TFT_BL
    ledcWrite(0, 25); // Dim to ~10%
#endif
  }

  vTaskDelay(10);
}
