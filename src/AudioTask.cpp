#include "AudioTask.h"
#include "Audio.h"
#include <SD.h>
#include <SPI.h>

Audio audio;

// Queue and Mutex handles
QueueHandle_t audioQueue;
SemaphoreHandle_t sdCardMutex;

// State Variables
AudioState currentState = AUDIO_IDLE;
int settingsVolume = 21; // The user-defined max volume
int fadeDurationMs = 1000;

// Variables for Fading Logic
unsigned long fadeStartTime = 0;
char nextFilename[64];

void audioTask(void *parameter) {
  // SD Card is already initialized in main.cpp
  if (SD.cardType() == CARD_NONE) {
    Serial.println("SD Not Ready!");
    vTaskDelete(NULL);
  }

  // Initialize Audio
  audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
  audio.setVolume(settingsVolume);

  AudioCommand cmd;

  while (true) {
    // 1. Always process Audio Events
    audio.loop();

    // 2. Check Queue for Commands (Non-blocking check)
    if (xQueueReceive(audioQueue, &cmd, 0) == pdTRUE) {
      switch (cmd.type) {
      case CMD_PLAY:
        if (xSemaphoreTake(sdCardMutex, portMAX_DELAY)) {
          audio.connecttoFS(SD, cmd.filename);
          // When starting fresh, jump to volume or fade in?
          // Simplified: Jump to volume, State PLAYING
          audio.setVolume(settingsVolume);
          currentState = AUDIO_PLAYING;
          xSemaphoreGive(sdCardMutex);
        }
        break;

      case CMD_STOP:
        // Soft Stop: Trigger fade out then stop
        fadeStartTime = millis();
        fadeDurationMs = 500; // Fast fade out
        currentState = AUDIO_STOPPING;
        break;

      case CMD_SET_VOLUME:
        settingsVolume = cmd.value;
        // If we are just playing, update immediately.
        // If fading, the fade logic will pick it up as target.
        if (currentState == AUDIO_PLAYING) {
          audio.setVolume(settingsVolume);
        }
        break;

      case CMD_CROSSFADE:
        if (currentState == AUDIO_PLAYING || currentState == AUDIO_FADING_IN) {
          // Start Fading OUT
          strncpy(nextFilename, cmd.filename, sizeof(nextFilename) - 1);
          nextFilename[sizeof(nextFilename) - 1] = '\0'; // Ensure Null Term

          fadeStartTime = millis();
          if (cmd.value > 0)
            fadeDurationMs = cmd.value;
          currentState = AUDIO_FADING_OUT;
        } else {
          // If idle, just play
          if (xSemaphoreTake(sdCardMutex, portMAX_DELAY)) {
            audio.connecttoFS(SD, cmd.filename);
            audio.setVolume(settingsVolume);
            currentState = AUDIO_PLAYING;
            xSemaphoreGive(sdCardMutex);
          }
        }
        break;
      }
    }

    // 3. State Machine Logic
    unsigned long now = millis();

    switch (currentState) {
    case AUDIO_STOPPING: {
      // ... (No Change needed here usually, but keeping context check)
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= fadeDurationMs) {
        audio.setVolume(0);
        audio.stopSong();
        currentState = AUDIO_IDLE;
      } else {
        float progress = (float)elapsed / fadeDurationMs;
        int newVol = settingsVolume * (1.0f - progress);
        if (newVol < 0)
          newVol = 0;
        audio.setVolume(newVol);
      }
      break;
    }

    case AUDIO_FADING_OUT: {
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= fadeDurationMs / 2) {

        // FADE OUT COMPLETE
        audio.setVolume(0);

        // Load Next Song - CRITICAL MUTEX FIX
        // Wait Indefinitely (or very long) to ensure we don't drop the
        // transition
        if (xSemaphoreTake(sdCardMutex, portMAX_DELAY)) {
          audio.connecttoFS(SD, nextFilename);
          xSemaphoreGive(sdCardMutex);

          // Start Fading IN
          currentState = AUDIO_FADING_IN;
          fadeStartTime = now;
        } else {
          // Should only happen if max delay expires (approx 49 days)
          // or system failure. Go to Idle.
          currentState = AUDIO_IDLE;
        }
      } else {
        // ... (Fade logic)
        float progress = (float)elapsed / (fadeDurationMs / 2.0f);
        int newVol = settingsVolume * (1.0f - progress);
        if (newVol < 0)
          newVol = 0;
        audio.setVolume(newVol);
      }
      break;
    }

    case AUDIO_FADING_IN: {
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= fadeDurationMs / 2) {
        // FADE IN COMPLETE
        audio.setVolume(settingsVolume);
        currentState = AUDIO_PLAYING;
      } else {
        // Map elapsed [0 to Duration/2] -> [0 to settingsVolume]
        float progress = (float)elapsed / (fadeDurationMs / 2.0f);
        int newVol = settingsVolume * progress;
        if (newVol > settingsVolume)
          newVol = settingsVolume;
        audio.setVolume(newVol);
      }
      break;
    }

    case AUDIO_PLAYING:
    case AUDIO_IDLE:
    default:
      break;
    }

    // Yield slightly to prevent Watchdog (but keep it small for audio
    // responsiveness)
    vTaskDelay(1);
  }
}
