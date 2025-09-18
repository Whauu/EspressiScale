#include "BLE.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_gatt_common_api.h"

// ============================
// globals
// ============================
BLEServer*          pServer              = nullptr;
BLECharacteristic*  pReadCharacteristic  = nullptr;
BLECharacteristic*  pWriteCharacteristic = nullptr;
bool                deviceConnected      = false;

const byte modelByte = 0x03;

unsigned long lastWeightNotifyTime = 0;
const long    weightNotifyInterval = 120;
int           i_onWrite_counter    = 0;

int   windowLength = 5;
float circularBuffer[5];
int   bufferIndex = 0;

// ============================
// OTA over BLE implementation
// ============================
namespace EspressiOtaBLE {

  // Internal state
  static State    g_state       = State::Idle;
  static Target   g_target      = Target::App;
  static size_t   g_totalSize   = 0;
  static size_t   g_written     = 0;
  static bool     g_begun       = false;
  static bool     g_shaProvided = false;
  static uint8_t  g_shaExpected[32];

  // Queue/Task handles
  static QueueHandle_t s_otaQ      = nullptr;
  static TaskHandle_t  s_otaTask   = nullptr;
  static volatile bool s_otaActive = false;
  static uint32_t      s_lastNotifyMs = 0;
  static const uint32_t NOTIFY_INTERVAL_MS = 10; // notify every 500ms
  static size_t   s_lastNotifiedWritten = 0;

  // ---- forward declarations used by otaWriterTask ----
  static bool Finalize();
  static void SetState(State s, const char* msg);
  static void NotifyStatus(const char* stateName, const char* msg);

  struct OtaChunk { uint8_t* data; size_t len; };

  enum MsgType : uint8_t { MSG_START, MSG_CHUNK, MSG_END, MSG_ABORT };

  struct OtaMsg {
    MsgType type;
    uint8_t* data;   // for CHUNK: heap copy of payload; for START: 4B size + 1B target
    size_t   len;    // payload length
  };

  // ---- status getters
  State  GetState()     { return g_state; }
  size_t GetTotalSize() { return g_totalSize; }
  size_t GetWritten()   { return g_written; }

  // ---- notify progress on your READ characteristic (FFF4)
  static void NotifyStatus(const char* stateName, const char* msg) {
    if (!pReadCharacteristic) return;
    char buf[196];
    snprintf(buf, sizeof(buf),
      "{\"state\":\"%s\",\"written\":%u,\"total\":%u,\"msg\":\"%s\"}",
      stateName, (unsigned)g_written, (unsigned)g_totalSize, msg ? msg : "");
    pReadCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    pReadCharacteristic->notify();
  }


  // ---- Notify if an OTA is active
  bool IsActive() { return s_otaActive; }


  static void otaWriterTask(void* /*arg*/) {
  Target   curTarget   = Target::App;
  uint32_t totalSizeLE = 0;

  for (;;) {
    OtaMsg m{};
    if (xQueueReceive(s_otaQ, &m, portMAX_DELAY) != pdTRUE) continue;

    if (m.type == MSG_START) {
      // Parse target + total size from msg->data
      if (m.len < 5) { SetState(State::Error, "START msg too short"); goto release; }

      curTarget   = (m.data[0] <= 1) ? (Target)m.data[0] : Target::App;
      totalSizeLE = (uint32_t)m.data[1] | ((uint32_t)m.data[2]<<8) |
                    ((uint32_t)m.data[3]<<16) | ((uint32_t)m.data[4]<<24);
      free(m.data); m.data = nullptr;

      // Begin update here (NOT in BLE callback)
      g_target    = curTarget;
      g_totalSize = totalSizeLE;
      g_written   = 0;
      g_begun     = false;

      const int cmd = (g_target == Target::Filesystem) ? U_SPIFFS : U_FLASH;
      if (!Update.begin(g_totalSize, cmd)) {
        SetState(State::Error, "Update.begin failed");
        s_otaActive = false;
        continue;
      }
      s_otaActive = true;
      s_lastNotifyMs = millis();
      SetState(State::Preparing, "OK");
      s_lastNotifiedWritten = 0;
      continue;
    }

    if (m.type == MSG_CHUNK) {
      if (!s_otaActive || (g_state != State::Preparing && g_state != State::Writing)) {
        if (m.data) free(m.data);
        SetState(State::Error, "Not ready");
        continue;
      }
      if (!m.data || m.len == 0) { /* ignore */ continue; }

      if (!g_begun) g_state = State::Writing;

      // Bounds check: don't allow more than announced total
      if (g_written + m.len > g_totalSize) {
         if (m.data) free(m.data);
         SetState(State::Error, "Size overflow");
         s_otaActive = false;
         continue;
      }

      if (g_written == 0 && m.len > 0) {
        // Log the first 8 bytes we are about to write
        char dbg[64]; int n = 0;
        n += snprintf(dbg + n, sizeof(dbg) - n, "CH0:");
        for (size_t i = 0; i < m.len && i < 8; ++i) n += snprintf(dbg + n, sizeof(dbg) - n, "%02X", m.data[i]);
        SetState(State::Preparing, dbg);

        // Warn-only: don't abort even if not 0xE9; let Update.write decide.
        if (m.data[0] != 0xE9) {
            SetState(State::Preparing, "CH0:WARN_non_E9_first_byte");
        }
      }

      size_t w = Update.write(m.data, m.len);
      free(m.data); m.data = nullptr;

      if (w != m.len) {
        SetState(State::Error, "Write short");
        s_otaActive = false;
        // Drain remaining queued data to avoid leaks
        OtaMsg drain{};
        while (xQueueReceive(s_otaQ, &drain, 0) == pdTRUE) {
          if (drain.data) free(drain.data);
        }
        continue;
      }

      g_begun   = true;
      g_written += w;

      uint32_t now = millis();
      const bool queueBusy = (s_otaQ && uxQueueMessagesWaiting(s_otaQ) >= 6);
      if ((g_written - s_lastNotifiedWritten) >= 4096 || queueBusy) {
        NotifyStatus("Writing", "");
        s_lastNotifyMs       = now;
        s_lastNotifiedWritten = g_written;
      } else if (now - s_lastNotifyMs >= NOTIFY_INTERVAL_MS) {
        NotifyStatus("Writing", "");
        s_lastNotifyMs       = now;
        s_lastNotifiedWritten = g_written;
      }
      continue;
    }

    if (m.type == MSG_END) {
      if (!s_otaActive) continue;
      g_state = State::Verifying;
      if (g_written != g_totalSize) {
         SetState(State::Error, "Size mismatch (written != total)");
         s_otaActive = false;
         // Optional: Update.abort();
         continue;
      }

      if (!Update.end(true)) {
        SetState(State::Error, "Update.end failed");
        s_otaActive = false;
        continue;
      }
      SetState(State::Finished, "Rebooting");
      vTaskDelay(pdMS_TO_TICKS(300));
      ESP.restart();
      continue;
    }

    if (m.type == MSG_ABORT) {
      s_otaActive = false;
      if (Update.isRunning()) Update.abort();
      // Drain queue
      OtaMsg drain{};
      while (xQueueReceive(s_otaQ, &drain, 0) == pdTRUE) {
        if (drain.data) free(drain.data);
      }
      g_totalSize = 0; g_written = 0; g_begun = false;
      SetState(State::Idle, "Aborted");
      continue;
    }

release:
    if (m.data) free(m.data);
  }
}


  static void startOtaWorkerIfNeeded() {
  if (!s_otaQ)  s_otaQ  = xQueueCreate(24, sizeof(OtaMsg)); // was 12
  if (!s_otaTask)
    xTaskCreatePinnedToCore(otaWriterTask, "ota_writer", 12288, nullptr, 5, &s_otaTask, 1); // was 8192,5
  }



  static void SetState(State s, const char* msg) {
    g_state = s;
    const char* name =
      (s==State::Idle?"Idle":s==State::Preparing?"Preparing":s==State::Writing?"Writing":
       s==State::Verifying?"Verifying":s==State::Finished?"Finished":"Error");
    NotifyStatus(name, msg);
  }

  static bool Finalize() {
    g_state = State::Verifying;
    // Validate and set next boot partition
    if (!Update.end(true)) {
      // Provide detail in msg if possible
      SetState(State::Error, "Update.end failed");
      return false;
    }
    return true;
  }

  bool HandleWriteFrameRaw(const uint8_t* data, size_t len) {
  if (!data || len == 0) return false;
  if (data[0] != kMagic) return false; // not OTA

  if (len < 2) { /* too short, but consume */ return true; }
  const uint8_t cmd = data[1];

  startOtaWorkerIfNeeded(); // ensure worker exists

  if (cmd == kSTART) {
    if (len < 2 + 1 + 4) { /* consume; worker will error when it sees bad len */ return true; }
    // Pack target + sizeLE4 into a tiny heap buffer (5 bytes)
    uint8_t* startPayload = (uint8_t*)malloc(5);
    if (!startPayload) return true;
    startPayload[0] = data[2];
    memcpy(&startPayload[1], &data[3], 4);
    OtaMsg m{ MSG_START, startPayload, 5 };
    // **ZERO timeout** to avoid blocking BTC_TASK
    if (xQueueSend(s_otaQ, &m, 0) != pdTRUE) { free(startPayload); /* drop */ }
    return true;
  }

  if (cmd == kCHUNK) {
    if (len <= 2) return true;
    const size_t payloadLen = len - 2;
    uint8_t* copy = (uint8_t*)malloc(payloadLen);
    if (!copy) return true;
    memcpy(copy, &data[2], payloadLen);
    OtaMsg m{ MSG_CHUNK, copy, payloadLen };
    if (xQueueSend(s_otaQ, &m, 0) != pdTRUE) { free(copy); /* drop */ }
    return true;
  }

  if (cmd == kEND) {
    OtaMsg m{ MSG_END, nullptr, 0 };
    (void)xQueueSend(s_otaQ, &m, 0);
    return true;
  }

  if (cmd == kABORT) {
    OtaMsg m{ MSG_ABORT, nullptr, 0 };
    (void)xQueueSend(s_otaQ, &m, 0);
    return true;
  }

  // Unknown OTA cmd: consume silently (no work in BT context)
  return true;
}


  bool HandleWriteFrame(const std::string& v) {
    if (v.empty()) return false;
    // Safe: use raw handler
    return HandleWriteFrameRaw(reinterpret_cast<const uint8_t*>(v.data()), v.size());
  }

  // -------- Optional callback wrapper (only if you don't have your own) --------
  class _WriteCB : public BLECharacteristicCallbacks {
   public:
    void onWrite(BLECharacteristic* ch) override {
      if (!ch) return;
      size_t len = ch->getLength();
      const uint8_t* data = (const uint8_t*)ch->getData();
      if (HandleWriteFrameRaw(data, len)) return; // OTA consumed
      // Non-OTA -> weak fallback
      LegacyOnWriteFallback(data, len);
    }
  };

  static _WriteCB g_cb;

  void AttachWriteCallbackIfNeeded() {
    if (pWriteCharacteristic) {
      pWriteCharacteristic->setCallbacks(&g_cb);
    }
  }

  // Weak fallbacks (no-op by default)
  void LegacyOnWriteFallback(const uint8_t* /*data*/, size_t /*len*/) {}
  void LegacyOnWriteFallback(const std::string& /*v*/) {}

}