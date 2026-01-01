#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct SystemSettings {
  int volume;
  int fadeTimeMs;
  bool useCrossfade;
  int currentPresetIndex;
  int screenBrightness;
  bool isDarkMode;
};

class SettingsManager {
public:
  SettingsManager() {}

  void begin() {
    prefs.begin("padium", false); // Read/Write
  }

  SystemSettings load() {
    SystemSettings s;
    // Open in Read-Only to load limits write cycles if we were just reading
    // But here we keep it open or just open briefly.
    // For simplicity, let's just use the member prefs which is initialized in
    // begin()

    // Actually begin() opens the namespace.
    // We can just read. Use defaults if missing.
    s.volume = prefs.getInt("vol", 21);
    s.fadeTimeMs = prefs.getInt("fade", 1000);
    s.useCrossfade = prefs.getBool("xfade", true);
    s.currentPresetIndex = prefs.getInt("preset", 0);
    s.screenBrightness = prefs.getInt("bright", 255);
    s.isDarkMode = prefs.getBool("theme", true);

    currentSettings = s;
    return s;
  }

  void save(const SystemSettings &s) {
    if (s.volume != currentSettings.volume)
      prefs.putInt("vol", s.volume);
    if (s.fadeTimeMs != currentSettings.fadeTimeMs)
      prefs.putInt("fade", s.fadeTimeMs);
    if (s.useCrossfade != currentSettings.useCrossfade)
      prefs.putBool("xfade", s.useCrossfade);
    if (s.currentPresetIndex != currentSettings.currentPresetIndex)
      prefs.putInt("preset", s.currentPresetIndex);
    if (s.screenBrightness != currentSettings.screenBrightness)
      prefs.putInt("bright", s.screenBrightness);
    if (s.isDarkMode != currentSettings.isDarkMode)
      prefs.putBool("theme", s.isDarkMode);

    currentSettings = s;
  }

  // Helper to update single value without full struct if needed,
  // but full struct save is cleaner for usage.

private:
  Preferences prefs;
  SystemSettings currentSettings;
};

#endif
