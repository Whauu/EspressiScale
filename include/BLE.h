#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <string>

// ===== Your existing UUIDs (unchanged) =====
#define CUUID_ESPRESSISCALE_READ      "0000FFF4-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITE     "000036F5-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITEBACK "83CDC3D4-3BA2-13FC-CC5E-106C351A9352"
#define SUUID_ESPRESSISCALE           "0000FFF0-0000-1000-8000-00805F9B34FB"

// ===== Your existing globals (now as extern; defined in BLE.cpp) =====
extern BLEServer*        pServer;
extern BLECharacteristic* pReadCharacteristic;
extern BLECharacteristic* pWriteCharacteristic;
extern bool              deviceConnected;

extern const byte        modelByte;

// ble timing / filters you already had
extern unsigned long     lastWeightNotifyTime;  // last notification time
extern const long        weightNotifyInterval;  // ms between weight notifies
extern int               i_onWrite_counter;

// smoothing buffer config (unchanged)
extern int               windowLength;          // default window length
extern float             circularBuffer[5];
extern int               bufferIndex;


// ===================================================================================
//  OTA over BLE (header API) — reuses your existing READ/WRITE characteristics
//  Protocol (frames on WRITE char):
//    [0xFA, 0x01, target(0=app,1=fs), totalSize(4 LE), optional sha256(32B)]  -> START
//    [0xFA, 0x02, CHUNK_BYTES...]                                            -> CHUNK
//    [0xFA, 0x03]                                                            -> END
//    [0xFA, 0x04]                                                            -> ABORT
//
//  All other frames (not starting with 0xFA) fall through to your normal handler.
// ===================================================================================
namespace EspressiOtaBLE {

  enum class Target : uint8_t { App = 0, Filesystem = 1 };
  enum class State  : uint8_t { Idle, Preparing, Writing, Verifying, Finished, Error };

  // Minimal status queries (read-only)
  State  GetState();
  size_t GetTotalSize();
  size_t GetWritten();

  // Call this at the TOP of your existing onWrite() handler:
  //   std::string v = c->getValue();
  //   if (EspressiOtaBLE::HandleWriteFrame(v)) return; // OTA consumed it
  //
  // Returns true if it was an OTA frame & was handled; false = let your normal code run.
  bool   HandleWriteFrame(const std::string& v);

  // Optional: If you don't already attach your own write callback, you can call this
  // once after creating pWriteCharacteristic to attach a callback that only intercepts
  // OTA frames and forwards non-OTA frames to a weak fallback hook.
  void   AttachWriteCallbackIfNeeded();

  // Weak fallback for non-OTA writes when using AttachWriteCallbackIfNeeded().
  // You can implement this in one of your .cpp files if you want to handle
  // normal writes via that path. Otherwise, it’s a no-op.
  void   LegacyOnWriteFallback(const std::string& v) __attribute__((weak));

} // namespace EspressiOtaBLE

