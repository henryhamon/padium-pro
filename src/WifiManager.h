#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "AudioTask.h"
#include <Arduino.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>

// Forward Declaration if needed, but AudioTask.h provides the types.

class WifiManager {
public:
  WifiManager();
  void begin();
  void startAP(); // Starts AP, Stops Audio, Begins Server
  void stopAP();  // Stops Server, Stops AP, Calls scanPresets callback?
  void handleClient();

  String getIP(); // Encapsulate WiFi IP retrieval
  // Callback to refresh presets in main
  // Using a simple function pointer or external call.
  // Ideally main.cpp handles the logic after stopAP.

private:
  WebServer server;
  File uploadFile;
  String uploadTargetFolder;

  // Handlers
  void handleRoot();
  void handleCreate();
  void handleDelete();
  void handleUpload();
  void handleUploadLoop();

  // Helpers
  void deleteRecursive(File dir);
  void sendHeader();
  void sendFooter();
};

#endif
