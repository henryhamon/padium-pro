#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "Config.h"

// Commands for the Audio Queue
enum AudioCommandType { CMD_PLAY, CMD_STOP, CMD_CROSSFADE, CMD_SET_VOLUME };

enum AudioState {
  AUDIO_IDLE,
  AUDIO_PLAYING,
  AUDIO_FADING_OUT,
  AUDIO_FADING_IN,
  AUDIO_STOPPING
};

struct AudioCommand {
  AudioCommandType type;
  char filename[64]; // Path to file for Play/Crossfade
  int value;         // Volume (0-21) or other parameters
};

// Global Handles
extern QueueHandle_t audioQueue;
extern SemaphoreHandle_t sdCardMutex;

// Task Entry Point
void audioTask(void *parameter);

#endif
