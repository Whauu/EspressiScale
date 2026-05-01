#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Update.h>
#include <string>

// ===== EspressiScale UUIDs =====
#define CUUID_ESPRESSISCALE_READ      "0000FFF4-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITE     "000036F5-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITEBACK "83CDC3D4-3BA2-13FC-CC5E-106C351A9352"
#define SUUID_ESPRESSISCALE           "0000FFF0-0000-1000-8000-00805F9B34FB"

// ===== globals (extern; defined in BLE.cpp) =====
extern BLEServer*          pServer;
extern BLECharacteristic*  pReadCharacteristic;   // notify/progress out
extern BLECharacteristic*  pWriteCharacteristic;  // commands/data in
extern bool                deviceConnected;

extern const byte          modelByte;

extern unsigned long       lastWeightNotifyTime;
extern const long          weightNotifyInterval;
extern int                 i_onWrite_counter;

extern int                 windowLength;
extern float               circularBuffer[5];
extern int                 bufferIndex;


// ============================
// OTA over BLE (header API)
// ============================
namespace EspressiOtaBLE {

  // Magic + commands (sent to WRITE characteristic)
  static constexpr uint8_t kMagic = 0xFA;
  static constexpr uint8_t kSTART = 0x01; // [FA][01][target][size0..3][opt 32B sha256]
  static constexpr uint8_t kCHUNK = 0x02; // [FA][02][payload...]
  static constexpr uint8_t kEND   = 0x03; // [FA][03]
  static constexpr uint8_t kABORT = 0x04; // [FA][04]

  enum class Target : uint8_t { App = 0, Filesystem = 1 };
  enum class State  : uint8_t { Idle, Preparing, Writing, Verifying, Finished, Error };

  // Minimal status getters (optional)
  State  GetState();
  size_t GetTotalSize();
  size_t GetWritten();

  // Handle a write frame given raw bytes (preferred for your existing onWrite)
  // Returns true if it was an OTA frame and was consumed.
  bool HandleWriteFrameRaw(const uint8_t* data, size_t len);

  // Convenience: same as above but takes a std::string
  bool HandleWriteFrame(const std::string& v);

  // Check if an OTA is active (writing or verifying)
  bool IsActive();

  // Optional: attach a BLE write callback that only intercepts OTA frames and
  // forwards non-OTA frames to a weak fallback hook.
  void AttachWriteCallbackIfNeeded();

  // Weak fallback for non-OTA writes if using the attached callback.
  void LegacyOnWriteFallback(const uint8_t* data, size_t len) __attribute__((weak));
  void LegacyOnWriteFallback(const std::string& v)           __attribute__((weak));

}