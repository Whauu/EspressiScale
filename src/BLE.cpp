#include <Arduino.h>
#include "BLE.h"

// ===================================================================================
// Your original globals (definitions)
// ===================================================================================
BLEServer*         pServer             = nullptr;
BLECharacteristic* pReadCharacteristic = nullptr;
BLECharacteristic* pWriteCharacteristic= nullptr;
bool               deviceConnected     = false;

const byte modelByte = 0x03;

// ble
unsigned long lastWeightNotifyTime = 0;     // last time weight notification was sent
const long    weightNotifyInterval = 100;   // ms
int           i_onWrite_counter    = 0;

// smoothing buffer
int   windowLength = 5;                     // default window length
float circularBuffer[5];
int   bufferIndex = 0;


// ===================================================================================
// OTA over BLE implementation (reuses your existing UUIDs/chars)
// ===================================================================================
namespace EspressiOtaBLE {

  // Magic & commands
  static constexpr uint8_t kMagic = 0xFA;
  static constexpr uint8_t kSTART = 0x01;
  static constexpr uint8_t kCHUNK = 0x02;
  static constexpr uint8_t kEND   = 0x03;
  static constexpr uint8_t kABORT = 0x04;

  // Internal state
  static State   g_state       = State::Idle;
  static Target  g_target      = Target::App;
  static size_t  g_totalSize   = 0;
  static size_t  g_written     = 0;
  static bool    g_begun       = false;
  static bool    g_shaProvided = false;
  static uint8_t g_shaExpected[32]; // reserved; not enforced in this minimal block

  // ---- minimal status getters
  State  GetState()     { return g_state; }
  size_t GetTotalSize() { return g_totalSize; }
  size_t GetWritten()   { return g_written; }

  // ---- notify helpers (use your existing READ characteristic)
  static void NotifyStatus(const char* stateName, const char* msg) {
    if (!pReadCharacteristic) return;
    char buf[196];
    snprintf(buf, sizeof(buf),
      "{\"state\":\"%s\",\"written\":%u,\"total\":%u,\"msg\":\"%s\"}",
      stateName, (unsigned)g_written, (unsigned)g_totalSize, msg ? msg : "");
    pReadCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    pReadCharacteristic->notify();
  }

  static void SetState(State s, const char* msg) {
    g_state = s;
    const char* name =
      (s==State::Idle?"Idle":s==State::Preparing?"Preparing":s==State::Writing?"Writing":
       s==State::Verifying?"Verifying":s==State::Finished?"Finished":"Error");
    NotifyStatus(name, msg);
  }

  static bool StartUpdate(size_t total) {
    if (total == 0) return false;
    int cmd = (g_target == Target::Filesystem) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(total, cmd)) return false;
    g_begun = false;
    return true;
  }

  static bool Finalize() {
    g_state = State::Verifying;
    // Validates image and sets OTA boot partition
    if (!Update.end(true)) return false;
    return true;
  }

  bool HandleWriteFrame(const std::string& v)
  {
    if (v.empty() || (uint8_t)v[0] != kMagic) return false; // Not an OTA frame

    // OTA command stream
    if (v.size() >= 2) {
      const uint8_t cmd = (uint8_t)v[1];

      if (cmd == kSTART) {
        if (v.size() < 2 + 1 + 4) { SetState(State::Error, "START too short"); return true; }

        const uint8_t tgt = (uint8_t)v[2];
        if (tgt <= 1) g_target = (Target)tgt;

        const size_t total =
            (size_t)(uint8_t)v[3]       |
           ((size_t)(uint8_t)v[4] << 8) |
           ((size_t)(uint8_t)v[5] << 16)|
           ((size_t)(uint8_t)v[6] << 24);

        g_totalSize = total;
        g_written   = 0;

        g_shaProvided = (v.size() >= 2 + 1 + 4 + 32);
        if (g_shaProvided) memcpy(g_shaExpected, v.data() + (2+1+4), 32);

        if (!StartUpdate(g_totalSize)) { SetState(State::Error, "Update.begin failed"); return true; }
        SetState(State::Preparing, "OK");
        return true;
      }

      else if (cmd == kCHUNK) {
        if (g_state != State::Preparing && g_state != State::Writing) {
            SetState(State::Error, "Not ready"); return true;
        }
        if (v.size() <= 2) return true; // empty chunk -> ignore

        // Get mutable pointer into std::string storage (C++11 OK on Arduino)
        char* raw   = (char*)v.data();                 // non-const char*
        uint8_t* p  = (uint8_t*)(raw + 2);   // skip [0xFA, 0x02]
        size_t  n   = v.size() - 2;

        if (!g_begun) g_state = State::Writing;

        size_t w = Update.write(p, n);       // expects uint8_t*
        if (w != n) { SetState(State::Error, "Write short"); return true; }

        g_begun = true;
        g_written += w;
        NotifyStatus("Writing", "");
        return true;
        }


      else if (cmd == kEND) {
        if (!Finalize()) { SetState(State::Error, "Finalize failed"); return true; }
        SetState(State::Finished, "Rebooting");
        delay(300);
        ESP.restart(); // not reached
        return true;
      }

      else if (cmd == kABORT) {
        if (Update.isRunning()) Update.abort();
        g_totalSize = 0; g_written = 0; g_begun = false;
        SetState(State::Idle, "Aborted");
        return true;
      }
    }

    // Magic matched but unknown subcommand -> consume and flag error
    SetState(State::Error, "Unknown OTA cmd");
    return true;
  }

  // --------- Optional write-callback wrapper ----------
  class _EspressiOtaWriteCB : public BLECharacteristicCallbacks {
   public:
    void onWrite(BLECharacteristic* c) override {
      std::string v = c->getValue();
      if (HandleWriteFrame(v)) {
        // OTA consumed it; do not fall through
        return;
      }
      // Not an OTA frame -> call legacy hook (weak; may be defined by user)
      LegacyOnWriteFallback(v);
    }
  };

  void AttachWriteCallbackIfNeeded() {
    static _EspressiOtaWriteCB _cb;
    if (pWriteCharacteristic) {
      pWriteCharacteristic->setCallbacks(&_cb);
    }
  }

  // Default weak fallback (no-op). Implement in your own .cpp if you attach our callback.
  void LegacyOnWriteFallback(const std::string& /*v*/) {
    // Intentionally empty.
  }

} // namespace EspressiOtaBLE

