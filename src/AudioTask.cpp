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
  // Initialize SPI for SD (VSPI)
  SPIClass *vspi = new SPIClass(VSPI);
  vspi->begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  // Initialize SD Card
  if (!SD.begin(SD_CS, *vspi)) {
    Serial.println("SD Mount Failed!");
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
          strcpy(nextFilename, cmd.filename);
          fadeStartTime = millis();
          if (cmd.value > 0)
            fadeDurationMs = cmd.value;
          currentState = AUDIO_FADING_OUT;
        } else {
          // If idle, just play (fade in optionally, but simpler to just play)
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
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= fadeDurationMs) {
        audio.setVolume(0);
        audio.stopSong();
        currentState = AUDIO_IDLE;
      } else {
        // Linear Fade Down
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
      if (elapsed >=
          fadeDurationMs /
              2) { // Determine fade out time (e.g. half of transition or full?)
        // The prompt says "Implement rotary encoder... changing Fade Time".
        // Let's assume fadeDurationMs is the total crossfade time (FadeOut +
        // FadeIn)? Or FadeOut takes fadeDurationMs? "Crossfade" usually means
        // overlap, but here we only have 1 decoder. So it's: Fade Out -> Load
        // -> Fade In. Let's split fadeDurationMs into two halves: FadeOut and
        // FadeIn.

        // FADE OUT COMPLETE
        audio.setVolume(0);

        // Load Next Song
        if (xSemaphoreTake(sdCardMutex, 100)) { // Don't block forever
          audio.connecttoFS(SD, nextFilename);
          xSemaphoreGive(sdCardMutex);

          // Start Fading IN
          currentState = AUDIO_FADING_IN;
          fadeStartTime = now; // Reset timer for fade in
        } else {
          // Failed to take mutex? Abort or try next loop?
          // For safety, go to idle or retry. Let's IDLE.
          currentState = AUDIO_IDLE;
        }
      } else {
        // Calc Volume
        // Map elapsed [0 to Duration/2] -> [settingsVolume to 0]
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
