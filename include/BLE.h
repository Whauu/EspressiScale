#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define CUUID_ESPRESSISCALE_READ "0000FFF4-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITE "000036F5-0000-1000-8000-00805F9B34FB"
#define CUUID_ESPRESSISCALE_WRITEBACK "83CDC3D4-3BA2-13FC-CC5E-106C351A9352"
#define SUUID_ESPRESSISCALE "0000FFF0-0000-1000-8000-00805F9B34FB"

BLEServer *pServer = NULL;
BLECharacteristic *pReadCharacteristic = NULL;
BLECharacteristic *pWriteCharacteristic = NULL;
bool deviceConnected = false;

const byte modelByte = 0x03;

//ble
unsigned long lastWeightNotifyTime = 0;  // Stores the last time the weight notification was sent
const long weightNotifyInterval = 100;   // Interval at which to send weight notifications (milliseconds)
int i_onWrite_counter = 0;
//
int windowLength = 5;  // default window length
// define circular buffer
float circularBuffer[5];
int bufferIndex = 0;