#include "AudioTask.h"
#include "Config.h"
#include "UI_Logic.h"
#include <Arduino.h>

// --- Globals ---
UI_Controller ui;

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
int lastPotVal = -1;
unsigned long lastPotSendTime = 0;

bool isPlayingState = false;

// Settings & Encoder
int fadeTimeMs = 1000;
EditMode editMode = MODE_PRESET;
bool useCrossfade = true;
int lastClk = HIGH;

// Button Debounce & Hold Logic
unsigned long lastDebounceTime = 0;
unsigned long btnNextHoldTime = 0;
bool btnNextHeld = false;
unsigned long btnPrevHoldTime = 0;
bool btnPrevHeld = false;
const int HOLD_DELAY = 400;  // Time before repeat starts
const int REPEAT_RATE = 100; // Repeat every 100ms

// --- Helper Functions ---

void updateUI() {
  ui.update(keys[currentKeyIndex], keys[nextKeyIndex],
            presets[currentPresetIndex], volume, fadeTimeMs, editMode,
            useCrossfade);
}

void scanControls() {
  // 1. Potentiometer (Volume)
  int potVal = analogRead(PIN_POT);
  int newVol = map(potVal, 0, 4095, 0, 21);

  // Hysteresis Fix: Threshold > 1
  if (abs(newVol - volume) > 1) {
    if (millis() - lastPotSendTime > 100) {
      volume = newVol;
      AudioCommand cmd;
      cmd.type = CMD_SET_VOLUME;
      cmd.value = volume;
      xQueueSend(audioQueue, &cmd, 0);
      lastPotSendTime = millis();
      // UI update for volume can be implicit or explicit,
      // usually we update UI on every significant change
      updateUI();
    }
  }

  // 2. Rotary Encoder
  int currentClk = digitalRead(PIN_ENC_A);
  if (currentClk != lastClk && currentClk == LOW) { // Falling Edge
    int dtValue = digitalRead(PIN_ENC_B);
    int direction = (dtValue != currentClk) ? 1 : -1;

    switch (editMode) {
    case MODE_PRESET:
      currentPresetIndex += direction;
      if (currentPresetIndex < 0)
        currentPresetIndex = numPresets - 1;
      if (currentPresetIndex >= numPresets)
        currentPresetIndex = 0;
      break;
    case MODE_FADE_TIME:
      fadeTimeMs += (direction * 500);
      if (fadeTimeMs < 0)
        fadeTimeMs = 0;
      if (fadeTimeMs > 10000)
        fadeTimeMs = 10000;
      break;
    case MODE_TRANSITION:
      // Toggle Logic
      if (direction != 0)
        useCrossfade = !useCrossfade;
      break;
    }
    updateUI();
  }
  lastClk = currentClk;

  // Encoder Button (Toggle Mode)
  static int lastBtnState = HIGH;
  int btnState = digitalRead(PIN_ENC_BTN);
  if (lastBtnState == HIGH && btnState == LOW) { // Press
    delay(50);                                   // Debounce
    if (digitalRead(PIN_ENC_BTN) == LOW) {
      // Cycle Mode: 0 -> 1 -> 2 -> 0
      int m = (int)editMode;
      m = (m + 1) % 3;
      editMode = (EditMode)m;
      updateUI();
    }
  }
  lastBtnState = btnState;

  // 3. Navigation Buttons (Queue Only, Hold Logic)
  // NEXT Button
  if (digitalRead(PIN_NEXT) == LOW) {
    if (btnNextHoldTime == 0) { // First press
      btnNextHoldTime = millis();
      btnNextHeld = false;
      // Action: Jump 1
      nextKeyIndex = (nextKeyIndex + 1) % numKeys;
      updateUI();
    } else {
      // Holding
      if (millis() - btnNextHoldTime > HOLD_DELAY) {
        // Repeat action
        if (millis() - lastDebounceTime > REPEAT_RATE) {
          // WHOLE TONE JUMP (2 semitones)
          nextKeyIndex = (nextKeyIndex + 2) % numKeys;
          updateUI();
          lastDebounceTime = millis();
        }
      }
    }
  } else {
    btnNextHoldTime = 0;
  }

  // PREV Button
  if (digitalRead(PIN_PREV) == LOW) {
    if (btnPrevHoldTime == 0) {
      btnPrevHoldTime = millis();
      // Action: Jump 1 (Reverse)
      nextKeyIndex = (nextKeyIndex - 1 + numKeys) % numKeys;
      updateUI();
    } else {
      if (millis() - btnPrevHoldTime > HOLD_DELAY) {
        if (millis() - lastDebounceTime > REPEAT_RATE) {
          // WHOLE TONE JUMP (2 semitones)
          nextKeyIndex = (nextKeyIndex - 2 + numKeys) % numKeys;
          updateUI();
          lastDebounceTime = millis();
        }
      }
    }
  } else {
    btnPrevHoldTime = 0;
  }

  // 4. Play Button (Trigger)
  static int lastPlayState = HIGH;
  int playState = digitalRead(PIN_PLAY);
  if (lastPlayState == HIGH && playState == LOW) { // Press
    delay(50);
    if (digitalRead(PIN_PLAY) == LOW) {
      if (!isPlayingState) {
        // START
        currentKeyIndex = nextKeyIndex;
        AudioCommand cmd;
        cmd.type = CMD_PLAY;

        // File Safety: Replace '#' with 's'
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
        // ALREADY PLAYING
        if (nextKeyIndex != currentKeyIndex) {
          // TRANSITION
          currentKeyIndex = nextKeyIndex;
          AudioCommand cmd;
          if (useCrossfade) {
            cmd.type = CMD_CROSSFADE;
            cmd.value = fadeTimeMs;
          } else {
            // Hard Cut = Play new file immediately
            cmd.type = CMD_PLAY;
          }

          // File Safety
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
          // SAME KEY = STOP
          AudioCommand cmd;
          cmd.type = CMD_STOP;
          xQueueSend(audioQueue, &cmd, 0);
          isPlayingState = false;
        }
      }
      updateUI();
    }
  }
  lastPlayState = playState;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_PLAY, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);

  audioQueue = xQueueCreate(10, sizeof(AudioCommand));
  sdCardMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096 * 4, NULL, 2, NULL, 0);

  ui.init();
  updateUI();
}

void loop() {
  scanControls();
  vTaskDelay(10);
}
