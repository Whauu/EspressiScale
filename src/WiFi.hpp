#include <Arduino.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
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
#define FW_VERSION "1.4.1"
WebServer server(80);
HTTPUpdateServer httpUpdater;


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
  }
);

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
  
  
  // trigger the ESP32 to download & flash the new firmware
  server.on("/updateRemote", HTTP_GET, []() {
  HTTPClient httpFS;
  httpFS.begin(fs_URL); 
  if (httpFS.GET() != HTTP_CODE_OK) {
    server.send(500, "text/plain", "Failed getting filesystem update file");
    Serial.println("Failed getting LittleFS update file");
    httpFS.end();
    return;
  }
  int fsLen         = httpFS.getSize();
  WiFiClient* stream = httpFS.getStreamPtr();

  if (!Update.begin(fsLen, U_SPIFFS)) {
    server.send(500, "text/plain", "Failed starting FS update");
    Serial.println("Failed starting LittleFS update");
    httpFS.end();
    return;
  }
  size_t written = Update.writeStream(*stream);
  if (written != (size_t)fsLen || !Update.end(true)) {
    server.send(500, "text/plain", "Failed updating FS");
    Serial.println("Failed updating LittleFS");
    httpFS.end();
    return;
  }
  httpFS.end();
  Serial.println("LittleFS updated successfully");
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate verification for simplicity
    t_httpUpdate_return ret = httpUpdate.update(client, host, port, uri, FW_VERSION);
    switch (ret) {
      case HTTP_UPDATE_OK:
        // never reached; device reboots
        break;
      case HTTP_UPDATE_FAILED:
        server.send(500, "text/plain", "Update failed: " + String(httpUpdate.getLastError()));
        Serial.println("Failed updating firmware");
        break;
      case HTTP_UPDATE_NO_UPDATES:
        server.send(204, "text/plain", "No update available");
        break;
    }
  }
);
server.on("/updateRemoteBeta", HTTP_GET, []() {
  HTTPClient httpFS;
  httpFS.begin(fs_beta_URL); 
  if (httpFS.GET() != HTTP_CODE_OK) {
    server.send(500, "text/plain", "Failed getting filesystem update file");
    Serial.println("Failed getting LittleFS update file");
    httpFS.end();
    return;
  }
  int fsLen         = httpFS.getSize();
  WiFiClient* stream = httpFS.getStreamPtr();

  if (!Update.begin(fsLen, U_SPIFFS)) {
    server.send(500, "text/plain", "Failed starting FS update");
    Serial.println("Failed starting LittleFS update");
    httpFS.end();
    return;
  }
  size_t written = Update.writeStream(*stream);
  if (written != (size_t)fsLen || !Update.end(true)) {
    server.send(500, "text/plain", "Failed updating FS");
    Serial.println("Failed updating LittleFS");
    httpFS.end();
    return;
  }
  httpFS.end();
  Serial.println("LittleFS updated successfully");
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate verification for simplicity
    t_httpUpdate_return ret = httpUpdate.update(client, host, port, beta_uri, FW_VERSION);
    switch (ret) {
      case HTTP_UPDATE_OK:
        // never reached; device reboots
        break;
      case HTTP_UPDATE_FAILED:
        server.send(500, "text/plain", "Update failed: " + String(httpUpdate.getLastError()));
        Serial.println("Failed updating firmware");
        break;
      case HTTP_UPDATE_NO_UPDATES:
        server.send(204, "text/plain", "No update available");
        break;
    }
  }
);

  server.begin();

vTaskDelete(NULL);
}