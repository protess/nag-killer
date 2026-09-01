#include "remote_ota.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Update.h>
#include <WiFi.h>
#include <mbedtls/sha256.h>
#include <time.h>

#if __has_include("ota_config.h")
#include "ota_config.h"
#endif

#ifndef OTA_WIFI_SSID
#define OTA_WIFI_SSID ""
#endif

#ifndef OTA_WIFI_PASSWORD
#define OTA_WIFI_PASSWORD ""
#endif

#ifndef OTA_MANIFEST_URL
#if CONFIG_IDF_TARGET_ESP32
#define OTA_MANIFEST_URL \
  "https://raw.githubusercontent.com/protess/nag-killer/m5-atom/firmware/manifest.json"
#else
#define OTA_MANIFEST_URL \
  "https://raw.githubusercontent.com/protess/nag-killer/m5-atom/firmware/manifest-atoms3.json"
#endif
#endif

#ifndef OTA_BOARD_ID
#if CONFIG_IDF_TARGET_ESP32
#define OTA_BOARD_ID "m5stack-atom-lite"
#elif CONFIG_IDF_TARGET_ESP32S3
#define OTA_BOARD_ID "esp32s3"
#else
#define OTA_BOARD_ID "unsupported"
#endif
#endif

#ifndef OTA_CONNECT_TIMEOUT_MS
#define OTA_CONNECT_TIMEOUT_MS 20000UL
#endif

#ifndef OTA_READ_TIMEOUT_MS
#define OTA_READ_TIMEOUT_MS 20000UL
#endif

#ifndef OTA_MAX_MANIFEST_BYTES
#define OTA_MAX_MANIFEST_BYTES 4096
#endif

// Default trust anchor for raw.githubusercontent.com (Let's Encrypt). A
// private server can override OTA_ROOT_CA_PEM in ota_config.h.
static const char DEFAULT_OTA_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

#ifndef OTA_ROOT_CA_PEM
#define OTA_ROOT_CA_PEM DEFAULT_OTA_ROOT_CA
#endif

namespace RemoteOta {
namespace {

bool stationStarted = false;

void setError(String& error, const String& value) {
  error = value;
  Serial.printf("[REMOTE OTA] %s\n", error.c_str());
}

bool isHttpsUrl(const String& url) {
  return url.startsWith("https://") && url.length() > 12;
}

bool isLowerHexSha256(const String& value) {
  if (value.length() != 64) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

String jsonStringValue(const String& json, const char* key) {
  const String quotedKey = String('"') + key + '"';
  int pos = json.indexOf(quotedKey);
  if (pos < 0) return String();
  pos = json.indexOf(':', pos + quotedKey.length());
  if (pos < 0) return String();
  pos = json.indexOf('"', pos + 1);
  if (pos < 0) return String();

  String out;
  out.reserve(128);
  bool escaped = false;
  for (int i = pos + 1; i < (int)json.length(); ++i) {
    const char c = json[i];
    if (escaped) {
      if (c == '"' || c == '\\' || c == '/') out += c;
      else if (c == 'n') out += '\n';
      else if (c == 'r') out += '\r';
      else if (c == 't') out += '\t';
      else return String();
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      return out;
    } else {
      out += c;
    }
  }
  return String();
}

bool jsonSizeValue(const String& json, const char* key, size_t& value) {
  const String quotedKey = String('"') + key + '"';
  int pos = json.indexOf(quotedKey);
  if (pos < 0) return false;
  pos = json.indexOf(':', pos + quotedKey.length());
  if (pos < 0) return false;
  ++pos;
  while (pos < (int)json.length() && isspace((unsigned char)json[pos])) ++pos;
  if (pos >= (int)json.length() || !isdigit((unsigned char)json[pos])) return false;

  uint64_t parsed = 0;
  while (pos < (int)json.length() && isdigit((unsigned char)json[pos])) {
    parsed = parsed * 10 + (json[pos++] - '0');
    if (parsed > SIZE_MAX) return false;
  }
  value = (size_t)parsed;
  return value > 0;
}

String sha256Hex(const uint8_t digest[32]) {
  static const char HEX_CHARS[] = "0123456789abcdef";
  char out[65];
  for (size_t i = 0; i < 32; ++i) {
    out[i * 2] = HEX_CHARS[digest[i] >> 4];
    out[i * 2 + 1] = HEX_CHARS[digest[i] & 0x0F];
  }
  out[64] = '\0';
  return String(out);
}

bool waitForStation(String& error) {
  if (!isConfigured()) {
    setError(error, "OTA WiFi is not configured; copy ota_config.h.example to ota_config.h");
    return false;
  }
  beginStation();
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < OTA_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    setError(error, String("WiFi connect timeout, status=") + (int)WiFi.status());
    return false;
  }
  return true;
}

bool waitForValidClock(String& error) {
  static bool configured = false;
  if (!configured) {
    configTime(0, 0, "time.cloudflare.com", "pool.ntp.org");
    configured = true;
  }

  const uint32_t started = millis();
  const time_t minimumValidTime = 1704067200;  // 2024-01-01 UTC
  while (time(nullptr) < minimumValidTime && millis() - started < OTA_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  if (time(nullptr) < minimumValidTime) {
    setError(error, "NTP time sync failed; refusing TLS without a valid clock");
    return false;
  }
  return true;
}

bool beginHttps(HTTPClient& http, NetworkClientSecure& client,
                const String& url, String& error) {
  if (!isHttpsUrl(url)) {
    setError(error, "Only https:// OTA URLs are allowed");
    return false;
  }
  client.setCACert(OTA_ROOT_CA_PEM);
  client.setHandshakeTimeout(15);
  http.setConnectTimeout(15000);
  http.setTimeout(OTA_READ_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  if (!http.begin(client, url)) {
    setError(error, "HTTPClient begin failed");
    return false;
  }
  return true;
}

}  // namespace

void beginStation() {
  if (!isConfigured() || WiFi.status() == WL_CONNECTED || stationStarted) return;
  stationStarted = true;
  WiFi.setAutoReconnect(true);
  WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);
  Serial.printf("[REMOTE OTA] Connecting station to SSID '%s'\n", OTA_WIFI_SSID);
}

bool isConfigured() {
  return strlen(OTA_WIFI_SSID) > 0 && isHttpsUrl(String(OTA_MANIFEST_URL));
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String stationIp() {
  return isConnected() ? WiFi.localIP().toString() : String();
}

const char* manifestUrl() {
  return OTA_MANIFEST_URL;
}

int compareVersions(const String& left, const String& right) {
  size_t li = 0;
  size_t ri = 0;
  while (li < left.length() && !isdigit((unsigned char)left[li])) ++li;
  while (ri < right.length() && !isdigit((unsigned char)right[ri])) ++ri;

  for (uint8_t component = 0; component < 8; ++component) {
    uint32_t lv = 0;
    uint32_t rv = 0;
    while (li < left.length() && isdigit((unsigned char)left[li])) {
      lv = lv * 10 + (left[li++] - '0');
    }
    while (ri < right.length() && isdigit((unsigned char)right[ri])) {
      rv = rv * 10 + (right[ri++] - '0');
    }
    if (lv != rv) return lv < rv ? -1 : 1;

    while (li < left.length() && !isdigit((unsigned char)left[li])) ++li;
    while (ri < right.length() && !isdigit((unsigned char)right[ri])) ++ri;
    if (li >= left.length() && ri >= right.length()) return 0;
  }
  return left.compareTo(right);
}

bool fetchManifest(Manifest& manifest, String& error) {
  manifest = Manifest();
  error = String();
  if (!waitForStation(error) || !waitForValidClock(error)) return false;

  HTTPClient http;
  NetworkClientSecure client;
  if (!beginHttps(http, client, OTA_MANIFEST_URL, error)) return false;

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    setError(error, String("Manifest HTTP status ") + status);
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  if (contentLength <= 0 || contentLength > OTA_MAX_MANIFEST_BYTES) {
    setError(error, String("Invalid manifest size ") + contentLength);
    http.end();
    return false;
  }

  const String json = http.getString();
  http.end();
  if (json.length() != (size_t)contentLength || json.length() > OTA_MAX_MANIFEST_BYTES) {
    setError(error, "Manifest body was truncated or too large");
    return false;
  }

  manifest.version = jsonStringValue(json, "version");
  manifest.board = jsonStringValue(json, "board");
  manifest.firmwareUrl = jsonStringValue(json, "url");
  manifest.sha256 = jsonStringValue(json, "sha256");
  manifest.sha256.toLowerCase();
  if (!jsonSizeValue(json, "size", manifest.size)) {
    setError(error, "Manifest has an invalid or missing size");
    return false;
  }
  if (manifest.version.length() == 0) {
    setError(error, "Manifest has no version");
    return false;
  }
  if (manifest.board != OTA_BOARD_ID) {
    setError(error, String("Manifest board mismatch: expected ") + OTA_BOARD_ID +
                    " got " + manifest.board);
    return false;
  }
  if (!isHttpsUrl(manifest.firmwareUrl)) {
    setError(error, "Manifest firmware URL must use https://");
    return false;
  }
  if (!isLowerHexSha256(manifest.sha256)) {
    setError(error, "Manifest SHA-256 must be 64 hexadecimal characters");
    return false;
  }

  Serial.printf("[REMOTE OTA] Manifest version=%s board=%s size=%u\n",
                manifest.version.c_str(), manifest.board.c_str(),
                (unsigned)manifest.size);
  return true;
}

bool install(const Manifest& manifest, String& error, ProgressCallback progress) {
  error = String();
  if (!waitForStation(error) || !waitForValidClock(error)) return false;
  if (!isHttpsUrl(manifest.firmwareUrl) || !isLowerHexSha256(manifest.sha256) ||
      manifest.size == 0) {
    setError(error, "Refusing invalid manifest data");
    return false;
  }
  if (manifest.size > ESP.getFreeSketchSpace()) {
    setError(error, String("Firmware does not fit OTA partition: ") + manifest.size);
    return false;
  }

  HTTPClient http;
  NetworkClientSecure client;
  if (!beginHttps(http, client, manifest.firmwareUrl, error)) return false;
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    setError(error, String("Firmware HTTP status ") + status);
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  if (contentLength <= 0 || (size_t)contentLength != manifest.size) {
    setError(error, String("Firmware length mismatch: HTTP=") + contentLength +
                    " manifest=" + manifest.size);
    http.end();
    return false;
  }
  if (!Update.begin(manifest.size, U_FLASH)) {
    setError(error, String("Update.begin failed: ") + Update.errorString());
    http.end();
    return false;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  if (mbedtls_sha256_starts(&sha, 0) != 0) {
    Update.abort();
    http.end();
    mbedtls_sha256_free(&sha);
    setError(error, "SHA-256 initialization failed");
    return false;
  }

  NetworkClient* stream = http.getStreamPtr();
  uint8_t buffer[4096];
  size_t writtenTotal = 0;
  uint32_t lastDataMs = millis();
  bool ok = true;

  while (writtenTotal < manifest.size) {
    const size_t available = stream->available();
    if (available == 0) {
      if (!http.connected() || millis() - lastDataMs > OTA_READ_TIMEOUT_MS) {
        setError(error, "Firmware download timed out or disconnected");
        ok = false;
        break;
      }
      delay(1);
      continue;
    }

    const size_t remaining = manifest.size - writtenTotal;
    const size_t wanted = min(min(available, sizeof(buffer)), remaining);
    const int got = stream->readBytes(buffer, wanted);
    if (got <= 0) {
      setError(error, "Firmware stream read failed");
      ok = false;
      break;
    }
    if (writtenTotal == 0 && buffer[0] != 0xE9) {
      setError(error, "Downloaded file is not an ESP32 application image");
      ok = false;
      break;
    }
    if (mbedtls_sha256_update(&sha, buffer, got) != 0) {
      setError(error, "SHA-256 update failed");
      ok = false;
      break;
    }
    const size_t written = Update.write(buffer, got);
    if (written != (size_t)got) {
      setError(error, String("Flash write failed: ") + Update.errorString());
      ok = false;
      break;
    }
    writtenTotal += written;
    lastDataMs = millis();
    if (progress) progress(writtenTotal, manifest.size);
    delay(1);
  }

  uint8_t digest[32];
  if (ok && mbedtls_sha256_finish(&sha, digest) != 0) {
    setError(error, "SHA-256 finalization failed");
    ok = false;
  }
  mbedtls_sha256_free(&sha);
  http.end();

  if (ok && writtenTotal != manifest.size) {
    setError(error, "Downloaded byte count does not match manifest");
    ok = false;
  }
  if (ok) {
    const String actualSha = sha256Hex(digest);
    if (actualSha != manifest.sha256) {
      setError(error, String("SHA-256 mismatch: got ") + actualSha);
      ok = false;
    }
  }

  if (!ok) {
    Update.abort();
    Serial.println("[REMOTE OTA] Update aborted; current partition remains active");
    return false;
  }
  if (!Update.end(false)) {
    setError(error, String("Update.end verification failed: ") + Update.errorString());
    Update.abort();
    return false;
  }
  if (!Update.isFinished()) {
    setError(error, "Update did not finish cleanly");
    Update.abort();
    return false;
  }

  Serial.printf("[REMOTE OTA] Verified %u bytes, SHA-256=%s\n",
                (unsigned)writtenTotal, manifest.sha256.c_str());
  return true;
}

}  // namespace RemoteOta
