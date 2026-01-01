#include "AudioTask.h"
#include "Config.h"
#include "UI_Logic.h"
#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <algorithm>
#include <vector>

// --- Globals ---
UI_Controller ui;
Preferences prefs;
WebServer server(80);

// State Variables
int currentPresetIndex = 0;
// Dynamic Presets
std::vector<String> presetNames;

const char *keys[] = {"C",  "C#", "D",  "D#", "E",  "F",
                      "F#", "G",  "G#", "A",  "A#", "B"};
const int numKeys = 12;

int currentKeyIndex = 0;
int nextKeyIndex = 0;

int volume = 21;
bool isPlayingState = false;

// Settings
int fadeTimeMs = 1000;
bool useCrossfade = true;
int screenBrightness = 255;
bool isDarkMode = true;

// UI State Machine
enum UIState {
  VIEW_PERFORMANCE,
  VIEW_MENU,
  VIEW_WIFI // New State
};
UIState uiState = VIEW_PERFORMANCE;

int menuIndex = 0;
bool isMenuEditing = false;

// Controls
int lastClk = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long btnNextHoldTime = 0;
bool btnNextHeld = false;
unsigned long btnPrevHoldTime = 0;
bool btnPrevHeld = false;
unsigned long btnPlayHoldTime = 0;
const int HOLD_DELAY = 400;
const int REPEAT_RATE = 100;
const int PANIC_DELAY = 1000;

// Screensaver
unsigned long lastInteractionTime = 0;
bool isDimmed = false;

// Wi-Fi Upload File
File uploadFile;
String uploadTargetFolder = "/";

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

  // Safety Fallback
  if (presetNames.empty()) {
    presetNames.push_back("NO BANKS");
  }

  // Validate Index
  if (currentPresetIndex >= presetNames.size()) {
    currentPresetIndex = 0;
  }
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
    // Use SAFE accessor
    const char *pName = (presetNames.size() > 0)
                            ? presetNames[currentPresetIndex].c_str()
                            : "ERROR";

    ui.drawPerformance(keys[currentKeyIndex], keys[nextKeyIndex], pName, volume,
                       fadeTimeMs, useCrossfade);
  } else if (uiState == VIEW_WIFI) {
    ui.drawWifiScreen("Padium-Manager", WiFi.softAPIP().toString().c_str());
  } else {
    ui.drawMenu(menuIndex, isMenuEditing, fadeTimeMs, useCrossfade, isDarkMode,
                screenBrightness);
  }
}

// --- Wi-Fi / Web Server Logic ---

void deleteRecursive(File dir) {
  if (!dir)
    return;

  dir.rewindDirectory();
  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    String entryPath = entry.path();
    if (entry.isDirectory()) {
      deleteRecursive(entry);
      SD.rmdir(entryPath.c_str());
    } else {
      SD.remove(entryPath.c_str());
    }
    entry.close();
  }
}

String getFileListHTML() {
  String html =
      "<table border='1' cellspacing='0' cellpadding='5' width='100%'>";
  html += "<tr><th>Type</th><th>Name</th><th>Size</th><th>Action</th></tr>";

  // MUTEX LOCK
  xSemaphoreTake(sdCardMutex, portMAX_DELAY);

  File root = SD.open("/");
  if (!root) {
    xSemaphoreGive(sdCardMutex);
    return "Failed to open root";
  }

  root.rewindDirectory();
  while (true) {
    File entry = root.openNextFile();
    if (!entry)
      break;

    String entryName = entry.name();

    if (entryName.startsWith(".")) {
      entry.close();
      continue;
    }

    html += "<tr>";
    html += "<td>" + String(entry.isDirectory() ? "DIR" : "FILE") + "</td>";
    html += "<td>" + String(entryName) + "</td>";

    if (entry.isDirectory()) {
      html += "<td>-</td>";
    } else {
      html += "<td>" + String(entry.size()) + " B</td>";
    }

    html += "<td><a href='/delete?path=" + String(entry.path()) +
            "' onclick=\"return confirm('Delete " + String(entryName) +
            "?');\">[DELETE]</a></td>";
    html += "</tr>";
    entry.close();
  }
  root.close();

  // MUTEX UNLOCK
  xSemaphoreGive(sdCardMutex);

  html += "</table>";
  return html;
}

void handleRoot() {
  String html = "<html><head><title>Padium Pro Manager</title>";
  html +=
      "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif; margin:20px;} "
          "table{border-collapse:collapse;} th,td{text-align:left;}</style>";
  html += "<script>function setAction(form) { var folder = "
          "document.getElementById('targetFolder').value; form.action = "
          "'/upload?folder=' + folder; }</script>";
  html += "</head><body>";

  html += "<h1>Padium Pro File Manager</h1>";

  html += "<div style='background:#e0e0e0; padding:10px; margin-bottom:10px; "
          "border-radius:5px;'>";
  html += "<h4>Create New Bank</h4>";
  html += "<form method='POST' action='/create'>";
  html += "<input type='text' name='folderName' placeholder='Bank Name'>";
  html += "<input type='submit' value='Create'>";
  html += "</form></div>";

  html += "<div style='background:#f0f0f0; padding:15px; margin-bottom:20px; "
          "border-radius:5px;'>";
  html += "<h3>Upload File</h3>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data' "
          "onsubmit='setAction(this)'>";

  html += "Target Bank: <select id='targetFolder' name='targetFolder'>";

  // MUTEX LOCK for reading folders
  xSemaphoreTake(sdCardMutex, portMAX_DELAY);

  File root = SD.open("/");
  if (root) {
    root.rewindDirectory();
    while (true) {
      File entry = root.openNextFile();
      if (!entry)
        break;
      if (entry.isDirectory() && !String(entry.name()).startsWith(".")) {
        html += "<option value='" + String(entry.name()) + "'>" +
                String(entry.name()) + "</option>";
      }
      entry.close();
    }
    root.close();
  }

  xSemaphoreGive(sdCardMutex);

  html += "<option value='/'>Root (Not recommended)</option>";
  html += "</select><br><br>";

  html += "<input type='file' name='upload'>";
  html += "<input type='submit' value='Upload'>";
  html += "</form></div>";

  html += "<h3>SD Card Contents</h3>";
  html += getFileListHTML(); // Has internal Mutex
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleCreateFolder() {
  if (!server.hasArg("folderName")) {
    server.send(400, "text/plain", "Missing name");
    return;
  }
  String name = server.arg("folderName");
  if (name.length() > 0) {
    String path = "/" + name;

    xSemaphoreTake(sdCardMutex, portMAX_DELAY);
    if (!SD.exists(path)) {
      SD.mkdir(path);
    }
    xSemaphoreGive(sdCardMutex);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelete() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Missing path");
    return;
  }
  String path = server.arg("path");

  xSemaphoreTake(sdCardMutex, portMAX_DELAY);

  if (SD.exists(path)) {
    File f = SD.open(path);
    if (f.isDirectory()) {
      deleteRecursive(f);
      SD.rmdir(path.c_str());
    } else {
      SD.remove(path.c_str());
    }
    f.close();
  }

  xSemaphoreGive(sdCardMutex);

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpload() { server.send(200, "text/plain", ""); }

void handleUploadLoop() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (server.hasArg("folder")) {
      uploadTargetFolder = server.arg("folder");
      if (!uploadTargetFolder.startsWith("/"))
        uploadTargetFolder = "/" + uploadTargetFolder;
      if (!uploadTargetFolder.endsWith("/"))
        uploadTargetFolder += "/";
    } else {
      uploadTargetFolder = "/";
    }

    String filename = uploadTargetFolder + upload.filename;

    // Note: Upload loop takes place over many chunks. Mutex logic here is
    // tricky. But since we delay(600) on startWifiMode, audio is guaranteed
    // stopped. We will lock only file open/close if desired, but Audio won't
    // run anyway. Safest is to rely on Audio being STOPPED.

    xSemaphoreTake(sdCardMutex, portMAX_DELAY);
    if (SD.exists(filename.c_str()))
      SD.remove(filename.c_str());
    uploadFile = SD.open(filename.c_str(), FILE_WRITE);
    xSemaphoreGive(sdCardMutex);

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      xSemaphoreTake(sdCardMutex, portMAX_DELAY);
      uploadFile.write(upload.buf, upload.currentSize);
      xSemaphoreGive(sdCardMutex);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      xSemaphoreTake(sdCardMutex, portMAX_DELAY);
      uploadFile.close();
      xSemaphoreGive(sdCardMutex);
    }
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

void startWifiMode() {
  AudioCommand cmd;
  cmd.type = CMD_SET_VOLUME;
  cmd.value = 0;
  xQueueSend(audioQueue, &cmd, 0);
  cmd.type = CMD_STOP;
  xQueueSend(audioQueue, &cmd, 0);
  isPlayingState = false;

  // SAFETY DELAY for SPI Race Condition
  // Give AudioTask 600ms to finish fading (500ms) and release SPI Mutex
  delay(600);

  WiFi.softAP("Padium-Manager", "12345678");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/create", HTTP_POST, handleCreateFolder);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/upload", HTTP_POST, handleUpload, handleUploadLoop);
  server.begin();

  uiState = VIEW_WIFI;
  updateUI();
}

void stopWifiMode() {
  server.stop();
  WiFi.mode(WIFI_OFF);
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
        ui.applyTheme(isDarkMode);
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
  if (opt == MENU_WIFI) {
    startWifiMode();
    return;
  }
  if (opt == MENU_EXIT) {
    uiState = VIEW_PERFORMANCE;
    saveSettings();
    updateUI();
    return;
  }

  isMenuEditing = !isMenuEditing;
  if (!isMenuEditing)
    saveSettings();
  updateUI();
}

void scanControls() {
  if (uiState == VIEW_WIFI) {
    server.handleClient();
    if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
      delay(50);
      if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
        stopWifiMode();
        while (digitalRead(PIN_VOL_ENC_BTN) == LOW)
          delay(10);
      }
    }
    return;
  }

  static int lastVolClk = HIGH;
  int currentVolClk = digitalRead(PIN_VOL_ENC_A);
  if (currentVolClk != lastVolClk && currentVolClk == LOW) {
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
      if (uiState == VIEW_PERFORMANCE)
        updateUI();
    }
  }
  lastVolClk = currentVolClk;

  if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
    resetScreensaver();
    delay(50);
    if (digitalRead(PIN_VOL_ENC_BTN) == LOW) {
      if (uiState == VIEW_MENU) {
        uiState = VIEW_PERFORMANCE;
        isMenuEditing = false;
        saveSettings();
        updateUI();
      }
    }
  }

  int currentClk = digitalRead(PIN_ENC_A);
  if (currentClk != lastClk && currentClk == LOW) {
    resetScreensaver();
    int dtValue = digitalRead(PIN_ENC_B);
    int direction = (dtValue != currentClk) ? 1 : -1;

    if (uiState == VIEW_PERFORMANCE) {
      if (presetNames.size() > 0) {
        currentPresetIndex += direction;
        int maxIdx = presetNames.size();
        if (currentPresetIndex < 0)
          currentPresetIndex = maxIdx - 1;
        if (currentPresetIndex >= maxIdx)
          currentPresetIndex = 0;
        updateUI();
      }
    } else {
      handleMenuScroll(direction);
    }
  }
  lastClk = currentClk;

  static int lastBtnState = HIGH;
  int btnState = digitalRead(PIN_ENC_BTN);
  if (lastBtnState == HIGH && btnState == LOW) {
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

  if (uiState == VIEW_PERFORMANCE) {
    if (digitalRead(PIN_NEXT) == LOW) {
      resetScreensaver();
      if (btnNextHoldTime == 0) {
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

    int playState = digitalRead(PIN_PLAY);
    if (playState == LOW) {
      resetScreensaver();
      if (btnPlayHoldTime == 0) {
        btnPlayHoldTime = millis();
      } else {
        if (!isDimmed && (millis() - btnPlayHoldTime > PANIC_DELAY)) {
          AudioCommand cmd;
          cmd.type = CMD_SET_VOLUME;
          cmd.value = 0;
          xQueueSend(audioQueue, &cmd, 0);
          cmd.type = CMD_STOP;
          xQueueSend(audioQueue, &cmd, 0);
          isPlayingState = false;
          updateUI();
          while (digitalRead(PIN_PLAY) == LOW) {
            delay(10);
          }
          btnPlayHoldTime = 0;
          return;
        }
      }
    } else {
      if (btnPlayHoldTime > 0) {
        long holdDuration = millis() - btnPlayHoldTime;
        if (holdDuration < PANIC_DELAY) {
          if (presetNames.size() == 0 || presetNames[0] == "NO BANKS") {
          } else if (!isPlayingState) {
            currentKeyIndex = nextKeyIndex;
            AudioCommand cmd;
            cmd.type = CMD_PLAY;
            char safeKey[8];
            strcpy(safeKey, keys[currentKeyIndex]);
            if (safeKey[1] == '#') {
              safeKey[1] = 's';
              safeKey[2] = '\0';
            }
            String pName = presetNames[currentPresetIndex];
            sprintf(cmd.filename, "/%s/%s.mp3", pName.c_str(), safeKey);
            xQueueSend(audioQueue, &cmd, 0);
            isPlayingState = true;
          } else {
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
              String pName = presetNames[currentPresetIndex];
              sprintf(cmd.filename, "/%s/%s.mp3", pName.c_str(), safeKey);
              xQueueSend(audioQueue, &cmd, 0);
            } else {
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
  loadSettings();

  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);
  pinMode(PIN_VOL_ENC_A, INPUT);
  pinMode(PIN_VOL_ENC_B, INPUT);
  pinMode(PIN_VOL_ENC_BTN, INPUT);

#ifdef PIN_TFT_BL
  pinMode(PIN_TFT_BL, OUTPUT);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_TFT_BL, 0);
  updateBrightness();
#endif

  resetScreensaver();

  ui.init();
  ui.applyTheme(isDarkMode);
  ui.showSplashScreen();
  delay(1000);

  // Semaphore
  sdCardMutex = xSemaphoreCreateMutex();
  audioQueue = xQueueCreate(10, sizeof(AudioCommand));

  // Initialize SD with SPI manually? Setup above did simple SPI.
  // We need to ensure SD is ready before we lock Mutex for scanning.
  SPIClass tempSPI(VSPI);
  tempSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, tempSPI)) {
    ui.showErrorScreen("NO SD CARD");
    while (true) {
      delay(100);
    }
  }

  scanPresets(); // Now robust with Mutex checks (though mutex initialized right
                 // before)

  if (presetNames.size() == 0 || presetNames[0] == "NO BANKS") {
    ui.showErrorScreen("NO BANKS FOUND");
    delay(2000);
  }

  // NOTE: SD.end() was called in previous iterations but we need SD open for
  // scanning? scanPresets opens/closes root. AudioTask re-opens SD on its own
  // via its own SPI instance? Wait, AudioTask likely shares SPI bus. In IDF
  // sharing VSPI is okay if mutexed. The libraries (ESP32-audioI2S and regular
  // SD) might conflict if initializing twice. Best practice: Init SD once
  // globally. AudioTask assumes SD is available or inits it? Usually
  // Application keeps SD open. Let's remove SD.end() to avoid re-init issues,
  // OR ensure AudioTask inits it. Existing code had SD.end(). Let's keep
  // consistent for now unless it breaks.
  SD.end();

  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096 * 4, NULL, 2, NULL, 0);

  updateUI();
}

void loop() {
  scanControls();

  if (uiState != VIEW_WIFI && !isDimmed &&
      (millis() - lastInteractionTime > 30000)) {
    isDimmed = true;
#ifdef PIN_TFT_BL
    ledcWrite(0, 25);
#endif
  }

  vTaskDelay(10);
}
