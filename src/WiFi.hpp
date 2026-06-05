#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <scale.h>

WiFiManager wifiManager;
String header;

const char* fs_URL = "https://raw.githubusercontent.com/Whauu/EspressiScale_web/main/webflash/littlefs.bin";
const char* fs_beta_URL = "https://raw.githubusercontent.com/Whauu/EspressiScale_web/main/beta/littlefs.bin";
const char* versionURL  = "https://raw.githubusercontent.com/Whauu/EspressiScale_web/main/webflash/version.txt";
const char* versionBetaURL  = "https://raw.githubusercontent.com/Whauu/EspressiScale_web/main/beta/version.txt";
const char* host = "raw.githubusercontent.com";
const uint16_t port = 443;
const char* uri  = "/Whauu/EspressiScale_web/main/webflash/firmware.bin";
const char* beta_uri  = "/Whauu/EspressiScale_web/main/beta/firmware.bin";

#define pass "Espressi"
#define DNS_ADDRESS "espressiscale"
#define FW_VERSION "2.1.0"
WebServer server(80);
HTTPUpdateServer httpUpdater;

// ---------------------------------------------------------------------------
// OTA progress state — shared between the worker task and the HTTP handlers.
// `volatile` because the values are written by the OTA task and read by the
// webserver task. The String fields are only written while otaRunning toggles,
// so reading them from /updateProgress is safe enough for status reporting.
// ---------------------------------------------------------------------------
volatile bool    otaRunning = false;
volatile uint8_t otaPercent = 0;
String           otaStage   = "idle";   // downloading-fs | installing-fs |
                                        // downloading-fw | installing-fw |
                                        // rebooting | error | idle
String           otaError   = "";
bool             otaUseBeta = false;

static void otaProgressCb(size_t done, size_t total) {
  if (total == 0) return;
  otaPercent = (uint8_t)((done * 100) / total);
}

static void httpUpdateProgressCb(int done, int total) {
  if (total == 0) return;
  otaPercent = (uint8_t)(((int64_t)done * 100) / total);
}

// Runs in its own FreeRTOS task so the /updateRemote handler can return 202
// immediately and the browser can poll /updateProgress for live status.
static void otaTask(void* arg) {
  const char* fsUrl   = otaUseBeta ? fs_beta_URL : fs_URL;
  const char* fwUri   = otaUseBeta ? beta_uri    : uri;

  // -------- Phase 1: download + install LittleFS image --------
  otaStage   = "downloading-fs";
  otaPercent = 0;

  HTTPClient httpFS;
  httpFS.begin(fsUrl);
  httpFS.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int fsCode = httpFS.GET();
  if (fsCode != HTTP_CODE_OK) {
    otaStage   = "error";
    otaError   = "Failed getting filesystem update file (" + String(fsCode) + ")";
    Serial.println(otaError);
    httpFS.end();
    otaRunning = false;
    vTaskDelete(NULL);
    return;
  }

  int fsLen = httpFS.getSize();
  WiFiClient* stream = httpFS.getStreamPtr();

  if (!Update.begin(fsLen, U_SPIFFS)) {
    otaStage   = "error";
    otaError   = "Failed starting FS update";
    Serial.println(otaError);
    httpFS.end();
    otaRunning = false;
    vTaskDelete(NULL);
    return;
  }

  otaStage   = "installing-fs";
  otaPercent = 0;
  Update.onProgress(otaProgressCb);

  size_t written = Update.writeStream(*stream);
  if (written != (size_t)fsLen || !Update.end(true)) {
    otaStage   = "error";
    otaError   = "Failed updating FS";
    Serial.println(otaError);
    httpFS.end();
    otaRunning = false;
    vTaskDelete(NULL);
    return;
  }
  httpFS.end();
  Serial.println("LittleFS updated successfully");

  // -------- Phase 2: download + install firmware image --------
  otaStage   = "downloading-fw";
  otaPercent = 0;

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.onProgress(httpUpdateProgressCb);
  // httpUpdate downloads and writes in one call; we expose it as a single
  // "downloading-fw" phase, then flip to "installing-fw" near the end.
  t_httpUpdate_return ret = httpUpdate.update(client, host, port, fwUri, FW_VERSION);
  switch (ret) {
    case HTTP_UPDATE_OK:
      otaStage   = "rebooting";
      otaPercent = 100;
      // device reboots automatically — task may not return
      break;
    case HTTP_UPDATE_FAILED:
      otaStage = "error";
      otaError = "Update failed: " + String(httpUpdate.getLastError()) +
                 " " + httpUpdate.getLastErrorString();
      Serial.println(otaError);
      break;
    case HTTP_UPDATE_NO_UPDATES:
      otaStage = "error";
      otaError = "No update available";
      Serial.println(otaError);
      break;
  }

  otaRunning = false;
  vTaskDelete(NULL);
}

static void startOtaTask(bool beta) {
  if (otaRunning) return;
  otaUseBeta = beta;
  otaRunning = true;
  otaPercent = 0;
  otaError   = "";
  otaStage   = "downloading-fs";
  xTaskCreatePinnedToCore(otaTask, "ota", 8192, NULL, 1, NULL, 1);
}

void startWifi(void * parameter){
  wifiManager.setConnectRetries(10);
  wifiManager.autoConnect("EspressiScale", pass);
  MDNS.begin(DNS_ADDRESS);
  LittleFS.begin();

  // serve static UI
  server.on("/", HTTP_GET, []() {
    auto f = LittleFS.open("/index.html", "r");
    server.streamFile(f, "text/html");
    f.close();
  }
);
  server.on("/calibrate", HTTP_GET, []() {
    auto f = LittleFS.open("/calibration.html", "r");
    server.streamFile(f, "text/html");
    f.close();
  }
);

  server.on("/doTare", HTTP_GET, []() {
    bool ok = tareScale();
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "Tare failed");
  });

  server.on("/doCalibration50", HTTP_GET, []() {
    bool ok = doCalibration(50);
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "Calibration failed");
  });

  server.on("/doCalibration100", HTTP_GET, []() {
    bool ok = doCalibration(100);
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "Calibration failed");
  });

  server.on("/doCalibration200", HTTP_GET, []() {
    bool ok = doCalibration(200);
    server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "Calibration failed");
  });

  server.on("/getFW", HTTP_GET, [](){
    server.send(200, "text/plain", FW_VERSION);
  });

  // client-side check: return the GitHub version string
  server.on("/checkRemote", HTTP_GET, []() {
    HTTPClient http;
    http.begin(versionURL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      String latest = http.getString();
      latest.trim();
      server.send(200, "text/plain", latest);
    } else {
      server.send(502, "text/plain", "error");
    }
    http.end();
  });

  server.on("/checkRemoteBeta", HTTP_GET, []() {
    HTTPClient http;
    http.begin(versionBetaURL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      String latest = http.getString();
      latest.trim();
      server.send(200, "text/plain", latest);
    } else {
      server.send(502, "text/plain", "error");
    }
    http.end();
  });

  // Kick off the OTA worker, return immediately so the browser can poll.
  server.on("/updateRemote", HTTP_GET, []() {
    if (otaRunning) {
      server.send(409, "text/plain", "Update already in progress");
      return;
    }
    startOtaTask(false);
    server.send(202, "text/plain", "Update started");
  });

  server.on("/updateRemoteBeta", HTTP_GET, []() {
    if (otaRunning) {
      server.send(409, "text/plain", "Update already in progress");
      return;
    }
    startOtaTask(true);
    server.send(202, "text/plain", "Update started");
  });

  // Live progress endpoint polled by the web UI.
  server.on("/updateProgress", HTTP_GET, []() {
    String err = otaError;
    // Escape backslashes and quotes for safe JSON embedding.
    err.replace("\\", "\\\\");
    err.replace("\"", "\\\"");

    String json = "{";
    json += "\"running\":";  json += (otaRunning ? "true" : "false");
    json += ",\"stage\":\""; json += otaStage; json += "\"";
    json += ",\"percent\":"; json += String(otaPercent);
    json += ",\"error\":\""; json += err; json += "\"";
    json += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });

  server.begin();

  vTaskDelete(NULL);
}
