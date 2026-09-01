#pragma once

#include <Arduino.h>

namespace RemoteOta {

struct Manifest {
  String version;
  String board;
  String firmwareUrl;
  String sha256;
  size_t size = 0;
};

using ProgressCallback = void (*)(size_t written, size_t total);

// Starts the configured station connection without blocking the local AP.
void beginStation();

bool isConfigured();
bool isConnected();
String stationIp();
const char* manifestUrl();

// Fetches and validates the remote manifest over authenticated HTTPS.
bool fetchManifest(Manifest& manifest, String& error);

// Streams the firmware into the inactive OTA partition. The new partition is
// selected only after the advertised size and SHA-256 both match.
bool install(const Manifest& manifest, String& error,
             ProgressCallback progress = nullptr);

// Numeric dotted-version comparison. Prefix text such as "v" or
// "NAG-KILLER-v" is ignored. Returns <0, 0, or >0.
int compareVersions(const String& left, const String& right);

}  // namespace RemoteOta
