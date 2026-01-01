#include "AudioTask.h"
#include "Config.h"
#include "InputManager.h"
#include "SettingsManager.h"
#include "UI_Logic.h"
#include "WifiManager.h" // NEW
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <algorithm>
#include <cstdio>
#include <vector>

// --- Globals ---
UI_Controller ui;
// WebServer removed from here
SPIClass *sdSPI = NULL;

// Managers
SettingsManager settingsMgr;
InputManager inputMgr;
WifiManager wifiMgr; // NEW
SystemSettings settings;

// State Variables
std::vector<String> presetNames;
const char *keys[] = {"C",  "C#", "D",  "D#", "E",  "F",
                      "F#", "G",  "G#", "A",  "A#", "B"};
const int numKeys = 12;

int currentKeyIndex = 0;
int nextKeyIndex = 0;
bool isPlayingState = false;

// UI State Machine
enum UIState { VIEW_PERFORMANCE, VIEW_MENU, VIEW_WIFI };
UIState uiState = VIEW_PERFORMANCE;

int menuIndex = 0;
bool isMenuEditing = false;

// Screensaver
unsigned long lastInteractionTime = 0;
bool isDimmed = false;

// --- Helper Functions ---

void resetScreensaver() {
  lastInteractionTime = millis();
  if (isDimmed) {
    isDimmed = false;
#ifdef PIN_TFT_BL
    ledcWrite(0, settings.screenBrightness);
#endif
  }
}

void updateBrightness() {
#ifdef PIN_TFT_BL
  ledcWrite(0, settings.screenBrightness);
#endif
}

void scanPresets() {
  presetNames.clear();

  // MUTEX LOCK
  xSemaphoreTake(sdCardMutex, portMAX_DELAY);

  File root = SD.open("/");
  if (root) {
    root.rewindDirectory();
    while (true) {
      File entry = root.openNextFile();
      if (!entry)
        break;

      if (entry.isDirectory()) {
        String dirName = entry.name();
        // Filter System Folders
        if (!dirName.startsWith(".") && !dirName.startsWith("System")) {
          presetNames.push_back(dirName);
        }
      }
      entry.close();
    }
    root.close();
  }

  // MUTEX UNLOCK
  xSemaphoreGive(sdCardMutex);

  std::sort(presetNames.begin(), presetNames.end());

  if (presetNames.empty()) {
    presetNames.push_back("NO BANKS");
  }

  // Validate Index
  if (settings.currentPresetIndex >= presetNames.size()) {
    settings.currentPresetIndex = 0;
  }
}

void updateUI() {
  if (uiState == VIEW_PERFORMANCE) {
    const char *pName = (presetNames.size() > 0)
                            ? presetNames[settings.currentPresetIndex].c_str()
                            : "ERROR";

    ui.drawPerformance(keys[currentKeyIndex], keys[nextKeyIndex], pName,
                       settings.volume, settings.fadeTimeMs,
                       settings.useCrossfade);
  } else if (uiState == VIEW_WIFI) {
    ui.drawWifiScreen("Padium-Manager", wifiMgr.getIP().c_str());
  } else {
    ui.drawMenu(menuIndex, isMenuEditing, settings.fadeTimeMs,
                settings.useCrossfade, settings.isDarkMode,
                settings.screenBrightness);
  }
}

void startWifiMode() {
  wifiMgr.startAP(); // encapsulated stop logic and AP start
  uiState = VIEW_WIFI;
  updateUI();
}

void stopWifiMode() {
  wifiMgr.stopAP();
  scanPresets();
  uiState = VIEW_PERFORMANCE;
  updateUI();
}

void handleMenuScroll(int direction) {
  if (!isMenuEditing) {
    menuIndex += direction;
    if (menuIndex < 0)
      menuIndex = MENU_COUNT - 1;
    if (menuIndex >= MENU_COUNT)
      menuIndex = 0;
  } else {
    MenuOption opt = (MenuOption)menuIndex;
    switch (opt) {
    case MENU_FADE_TIME:
      settings.fadeTimeMs += (direction * 500);
      if (settings.fadeTimeMs < 0)
        settings.fadeTimeMs = 0;
      if (settings.fadeTimeMs > 10000)
        settings.fadeTimeMs = 10000;
      break;
    case MENU_TRANSITION:
      if (direction != 0)
        settings.useCrossfade = !settings.useCrossfade;
      break;
    case MENU_THEME:
      if (direction != 0) {
        settings.isDarkMode = !settings.isDarkMode;
        ui.applyTheme(settings.isDarkMode);
      }
      break;
    case MENU_BRIGHTNESS:
      settings.screenBrightness += (direction * 25);
      if (settings.screenBrightness < 10)
        settings.screenBrightness = 10;
      if (settings.screenBrightness > 255)
        settings.screenBrightness = 255;
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
  if (opt == MENU_WIFI) {
    startWifiMode();
    return;
  }
  if (opt == MENU_EXIT) {
    uiState = VIEW_PERFORMANCE;
    settingsMgr.save(settings);
    updateUI();
    return;
  }

  isMenuEditing = !isMenuEditing;
  if (!isMenuEditing)
    settingsMgr.save(settings);
  updateUI();
}

void loopInput() {
  inputMgr.update();

  if (uiState == VIEW_WIFI) {
    wifiMgr.handleClient(); // Using manager
    if (inputMgr.wasVolBtnPressed()) {
      stopWifiMode();
    }
    return;
  }

  // Screensaver Wake
  if (inputMgr.getVolumeDelta() != 0 || inputMgr.getNavDelta() != 0 ||
      inputMgr.wasNavBtnPressed() || inputMgr.wasVolBtnPressed() ||
      inputMgr.wasNextPressed() || inputMgr.wasPrevPressed() ||
      inputMgr.wasPlayPressed()) {
    resetScreensaver();
  }

  // 1. Volume
  int vDelta = inputMgr.getVolumeDelta();
  if (vDelta != 0) {
    int newVol = settings.volume + vDelta;
    if (newVol < 0)
      newVol = 0;
    if (newVol > 21)
      newVol = 21;
    if (newVol != settings.volume) {
      settings.volume = newVol;
      AudioCommand cmd;
      cmd.type = CMD_SET_VOLUME;
      cmd.value = settings.volume;
      xQueueSend(audioQueue, &cmd, 0);
      if (uiState == VIEW_PERFORMANCE)
        updateUI();
    }
  }

  // 2. Volume Button (Back)
  if (inputMgr.wasVolBtnPressed()) {
    if (uiState == VIEW_MENU) {
      uiState = VIEW_PERFORMANCE;
      isMenuEditing = false;
      settingsMgr.save(settings);
      updateUI();
    }
  }

  // 3. Navigation
  int nDelta = inputMgr.getNavDelta();
  if (nDelta != 0) {
    if (uiState == VIEW_PERFORMANCE) {
      if (presetNames.size() > 0) {
        settings.currentPresetIndex += nDelta;
        int maxIdx = presetNames.size();
        if (settings.currentPresetIndex < 0)
          settings.currentPresetIndex = maxIdx - 1;
        if (settings.currentPresetIndex >= maxIdx)
          settings.currentPresetIndex = 0;
        updateUI();
      }
    } else {
      handleMenuScroll(nDelta);
    }
  }

  // 4. Nav Button (Select)
  if (inputMgr.wasNavBtnPressed()) {
    if (uiState == VIEW_PERFORMANCE) {
      uiState = VIEW_MENU;
      menuIndex = 0;
      isMenuEditing = false;
      updateUI();
    } else {
      handleMenuClick();
    }
  }

  if (uiState == VIEW_PERFORMANCE) {
    // 5. Next/Prev
    if (inputMgr.wasNextPressed() || inputMgr.isNextHeld()) {
      int jump = inputMgr.isNextHeld() ? 2 : 1;
      nextKeyIndex = (nextKeyIndex + jump) % numKeys;
      updateUI();
    }

    if (inputMgr.wasPrevPressed() || inputMgr.isPrevHeld()) {
      int jump = inputMgr.isPrevHeld() ? 2 : 1;
      nextKeyIndex = (nextKeyIndex - jump + numKeys) % numKeys;
      updateUI();
    }

    // 6. Play / Panic
    if (inputMgr.isPlayHeld()) {
      if (!isDimmed) {
        AudioCommand cmd;
        cmd.type = CMD_SET_VOLUME;
        cmd.value = 0;
        xQueueSend(audioQueue, &cmd, 0);
        cmd.type = CMD_STOP;
        xQueueSend(audioQueue, &cmd, 0);
        isPlayingState = false;
        updateUI();
        return;
      }
    }

    if (inputMgr.wasPlayPressed()) {
      if (presetNames.size() == 0 || presetNames[0] == "NO BANKS") {
        // No Action
      } else if (!isPlayingState) {
        // Start Play
        currentKeyIndex = nextKeyIndex;
        AudioCommand cmd;
        cmd.type = CMD_PLAY;
        char safeKey[8];
        strncpy(safeKey, keys[currentKeyIndex], 7);
        if (safeKey[1] == '#') {
          safeKey[1] = 's';
          safeKey[2] = '\0';
        }

        char filename[64];
        snprintf(filename, sizeof(filename), "/%s/%s.mp3",
                 presetNames[settings.currentPresetIndex].c_str(), safeKey);

        strncpy(cmd.filename, filename, sizeof(cmd.filename) - 1);
        xQueueSend(audioQueue, &cmd, 0);
        isPlayingState = true;
      } else {
        // Transition or Stop
        if (nextKeyIndex != currentKeyIndex) {
          currentKeyIndex = nextKeyIndex;
          AudioCommand cmd;
          if (settings.useCrossfade) {
            cmd.type = CMD_CROSSFADE;
            cmd.value = settings.fadeTimeMs;
          } else {
            cmd.type = CMD_PLAY;
          }

          char safeKey[8];
          strncpy(safeKey, keys[currentKeyIndex], 7);
          if (safeKey[1] == '#') {
            safeKey[1] = 's';
            safeKey[2] = '\0';
          }

          char filename[64];
          snprintf(filename, sizeof(filename), "/%s/%s.mp3",
                   presetNames[settings.currentPresetIndex].c_str(), safeKey);

          strncpy(cmd.filename, filename, sizeof(cmd.filename) - 1);
          xQueueSend(audioQueue, &cmd, 0);
        } else {
          // Stop
          AudioCommand cmd;
          cmd.type = CMD_STOP;
          xQueueSend(audioQueue, &cmd, 0);
          isPlayingState = false;
        }
      }
      updateUI();
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Init Settings
  settingsMgr.begin();
  settings = settingsMgr.load();

  // Init Inputs
  inputMgr.init();

  // Init Wifi
  wifiMgr.begin();

  // Init Brightness control
#ifdef PIN_TFT_BL
  pinMode(PIN_TFT_BL, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_TFT_BL, 0);
  updateBrightness();
#endif

  resetScreensaver();

  // Init UI
  ui.init();
  ui.applyTheme(settings.isDarkMode);
  ui.showSplashScreen();
  delay(1000);

  // RTOS
  sdCardMutex = xSemaphoreCreateMutex();
  audioQueue = xQueueCreate(10, sizeof(AudioCommand));

  // Global SD Init
  sdSPI = new SPIClass(VSPI);
  sdSPI->begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, *sdSPI)) {
    ui.showErrorScreen("NO SD CARD");
    while (true)
      delay(100);
  }

  scanPresets();

  if (presetNames.size() == 0 || presetNames[0] == "NO BANKS") {
    ui.showErrorScreen("NO BANKS FOUND");
    delay(2000);
  }

  // Task
  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096 * 4, NULL, 2, NULL, 0);

  updateUI();
}

void loop() {
  loopInput();

  if (uiState != VIEW_WIFI && !isDimmed &&
      (millis() - lastInteractionTime > 30000)) {
    isDimmed = true;
#ifdef PIN_TFT_BL
    ledcWrite(0, 25);
#endif
  }

  vTaskDelay(10);
}
