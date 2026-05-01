#include <Arduino.h>
#include <battery.h>
#include <scale.h>
#include <filter.h>
#include <jd9613.h>
#include <lvgl.h>
#include <pin_config.h>
#include <SPI.h>
#include <time.h>
#include <esp_sntp.h>
#define TOUCH_MODULES_CST_SELF
#include <TouchLib.h>
#include <Wire.h>
#include <BLE.h>
#include <WiFi.hpp>

extern "C" {
  #include "esp_gatt_common_api.h"
}

#ifndef BOARD_HAS_PSRAM
#error "Please turn on PSRAM option to OPI PSRAM"
#endif

// ============================
// globals
// ============================
static EventGroupHandle_t   touch_eg;
static const uint16_t       screenWidth      = 294 * 2;
static const uint16_t       screenHeight     = 126;
static const size_t         lv_buffer_size   = screenWidth * screenHeight * sizeof(lv_color_t);
static constexpr float      FLOW_MAX         = 2.5f;     
static constexpr int        FLOW_DOT_COUNT   = 18; 
static constexpr int        FLOW_WIDTH       = 294;    
static lv_disp_draw_buf_t   draw_buf;
static lv_color_t          *buf              = NULL;
lv_obj_t                   *label_weight     = NULL;
lv_obj_t                   *label_timer      = NULL;
lv_obj_t                   *label_battery    = NULL;
lv_obj_t                   *flow_label       = NULL;

static int                  timer            = 0;
static bool                 timer_running    = false;
static unsigned long        last_update      = 0;
float                       currentWeight    = 0.0;
static bool                 prevTouched      = false;
static unsigned long        touchStart       = 0;
float                       batteryStatus    = 3.2;
int                         version          = 0;
int                         subversion       = 0;
int                         patch            = 0;

// ---- Logo map ----
extern uint8_t espressiscale_left_map[];
extern uint8_t espressiscale_right_map[];

// ============================
// Display and Touch
// ============================
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS);

void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}

inline void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  // uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  int _w1 = 294 - area->x1;
  int _w2 = area->x2 - 294 + 1;

  if (_w1 > 0)
  {
    TFT_CS_0_L;
    lcd_PushColors_SoftRotation(area->x1,
                  area->y1,
                  _w1,
                  h,
                  (uint16_t *)&color_p->full,
                  2); // Horizontal display
    TFT_CS_0_H;
  }
  if (_w2 > 0)
  {
    TFT_CS_1_L;
    lcd_PushColors_SoftRotation(0,
                  area->y1,
                  _w2,
                  h,
                  (uint16_t *)&color_p->full,
                  1); // Horizontal display
    TFT_CS_1_H;
  }

  lv_disp_flush_ready(disp);
}

// Touchpad read function
static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch.read())
  {
    TP_Point t = touch.getPoint(0);
    int16_t x = 126 - t.x;
    int16_t y = x;
    x = t.y;
    data->point.x = x;
    t.x = x;
    data->point.y = y;
    t.y = y;

    /* Adjust black shadow areas. */
    if (t.x > 326)
      data->point.x = t.x - 32;

    if (t.x > 294 && t.x < 326)
      data->state = LV_INDEV_STATE_REL;
    else
      data->state = LV_INDEV_STATE_PR;
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ============================
// Deep Sleep
// ============================
const gpio_num_t holdPins[] = {
  GPIO_NUM_14,
  GPIO_NUM_15,
  GPIO_NUM_16,
  GPIO_NUM_17,
  GPIO_NUM_18
};

static void deep_sleep()
{
  for (auto pin : holdPins) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    gpio_hold_en(pin);
  }
  esp_deep_sleep_start();
}

// ============================
// Battery Icon Update
// ============================

void updateBatteryIcon(float pct)
{
  if (pct >= 4.1f)      lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_FULL);
  else if (pct >= 3.80f) lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_3);
  else if (pct >= 3.50f) lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_2);
  else if (pct >= 3.20f) lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_1);
  else                lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_EMPTY);
}


// ============================
// Flow Calculation and Display
// ============================

void updateFlowDots(float flow)
{
  if (flow_label == NULL) return;

  if (flow < 0.0f) flow = 0.0f;
  if (flow > FLOW_MAX) flow = FLOW_MAX;

  int dots = (int)((flow / FLOW_MAX) * FLOW_DOT_COUNT + 0.5f);
  if (dots < 0) dots = 0;
  if (dots > FLOW_DOT_COUNT) dots = FLOW_DOT_COUNT;

  String s = "";
  for (int i = 0; i < dots; i++) {
    s += "•";
  }

  lv_label_set_text(flow_label, s.c_str());

  // Color coding: green for good flow, yellow for low flow, red for no flow or overflow
  lv_color_t c;
  if (flow < 0.75f) {
    c = lv_color_hex(0xf800);   // red
  }
  else if (flow < 1.0f) {
    c = lv_color_hex(0xfde0);   // yellow
  }
  else if (flow <= 2.0f) {
    c = lv_color_hex(0x07e0);   // green
  }
  else if (flow <= 2.25f) {
    c = lv_color_hex(0xfde0);   // yellow
  }
  else {
    c = lv_color_hex(0xf800);   // red
  }
  lv_obj_set_style_text_color(flow_label, c, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_invalidate(flow_label);
}

void getFlow()
{
  static float lastWeightForFlow = currentWeight;
  static unsigned long lastFlowTime = millis();

  unsigned long currentTime = millis();
  unsigned long timeDelta = currentTime - lastFlowTime;

  if (timeDelta > 0) {
    float weightDelta = currentWeight - lastWeightForFlow;
    float flow = (weightDelta / (timeDelta / 1000.0f)); // grams per second
    updateFlowDots(flow);
    lastWeightForFlow = currentWeight;
    lastFlowTime = currentTime;
  }
}

// ============================
// Helper functions
// ============================
void splitVersionString(const String& versionStr, int& version, int& subversion, int& patch) {
  int firstDot = versionStr.indexOf('.');
  int secondDot = versionStr.indexOf('.', firstDot + 1);

  if (firstDot == -1 || secondDot == -1) {
    version = subversion = patch = 0;
    return;
  }

  version = versionStr.substring(0, firstDot).toInt();
  subversion = versionStr.substring(firstDot + 1, secondDot).toInt();
  patch = versionStr.substring(secondDot + 1).toInt();
}

// Calculate XOR checksum for outgoing data
uint8_t calculateXOR(uint8_t *data, size_t len) {
  uint8_t xorValue = 0x03; // Starting value for XOR as per your protocol
  for (size_t i = 1; i < len - 1; i++) { // Start at index 1; reserve last byte for checksum
    xorValue ^= data[i];
  }
  return xorValue;
}

// Encode offset into three bytes
void encodeOffset(int32_t value, byte &byte1, byte &byte2, byte &byte3) {
  uint32_t uvalue = static_cast<uint32_t>(value);
  byte1 = (byte)((uvalue >> 16) & 0xFF);
  byte2 = (byte)((uvalue >> 8) & 0xFF);
  byte3 = (byte)(uvalue & 0xFF);
}

// Decode offset from three bytes
float decodeOffset(byte byte1, byte byte2, byte byte3) {
  // Combine the three bytes into a 24-bit value
  int32_t value = (byte1 << 16) | (byte2 << 8) | byte3;
  
  // Handle sign extension for 24-bit signed integer
  if (value & 0x800000) {  // Check if the sign bit (bit 23) is set
    value = value - 0x1000000;  // Convert to proper negative value
  }
  
  return (float)value;
}

// Encode weight into two bytes
void encodeWeight(float weight, byte &byte1, byte &byte2) {
  int weightInt = (int)(weight * 10);  // Convert to an integer (weight in grams * 10)
  byte1 = (byte)((weightInt >> 8) & 0xFF);
  byte2 = (byte)(weightInt & 0xFF);
}

// ============================
// BLE Functions
// ============================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected.");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected.");
    // Restart advertising so that new clients can connect.
    pServer->getAdvertising()->start();
  }
};

// BLE Characteristic Callbacks for handling write requests
class MyCallbacks : public BLECharacteristicCallbacks {
  uint8_t calculateChecksum(uint8_t *data, size_t len) {
    uint8_t xorSum = 0;
    for (size_t i = 0; i < len - 1; i++) {
      xorSum ^= data[i];
    }
    return xorSum;
  }

  // Validate the checksum of the received data
  bool validateChecksum(uint8_t *data, size_t len) {
    if (len < 2) return false;  // At least one data byte and one checksum byte are required
    uint8_t expectedChecksum = data[len - 1];
    uint8_t calculatedChecksum = calculateChecksum(data, len);
    return expectedChecksum == calculatedChecksum;
  }

  // Send simple task response via BLE notification
  void sendBleTask(int taskNumber) {
  if (deviceConnected && !EspressiOtaBLE::IsActive()) {
    byte data[7];

    data[0] = modelByte;
    data[1] = 0xAA;  // Type byte for weight stable
    data[2] = taskNumber;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = calculateXOR(data, 6);  // Last byte is XOR validation

    pReadCharacteristic->setValue(data, 7);
    pReadCharacteristic->notify();
  }
}

// Send offset via BLE notification
void sendBleOffset(float offset) {
  if (deviceConnected && !EspressiOtaBLE::IsActive()) {
    byte data[7];
    byte byte1, byte2, byte3; 
    encodeOffset(offset, byte1, byte2, byte3);

    data[0] = modelByte;
    data[1] = 0x66;  // Type byte for offset
    data[2] = byte1;
    data[3] = byte2;
    data[4] = byte3;
    data[5] = 0x00;
    data[6] = calculateXOR(data, 6);  // Last byte is XOR validation

    pReadCharacteristic->setValue(data, 7);
    pReadCharacteristic->notify();
  }
}

// Send firmware version via BLE notification
void sendFWVersion(int version, int subversion, int patch) {
  if (deviceConnected && !EspressiOtaBLE::IsActive()) {
    byte data[7];

    data[0] = modelByte;
    data[1] = 0x21;  // Type byte for FW version
    data[2] = version;
    data[3] = subversion;
    data[4] = patch;
    data[5] = 0x00;
    data[6] = calculateXOR(data, 6);  // Last byte is XOR validation

    pReadCharacteristic->setValue(data, 7);
    pReadCharacteristic->notify();
  }
}

  // Handle write requests from the client
  void onWrite(BLECharacteristic *pWriteCharacteristic) {
    if (pWriteCharacteristic != nullptr) {
      size_t len = pWriteCharacteristic->getLength();
      uint8_t *data = (uint8_t *)pWriteCharacteristic->getData();

      if (EspressiOtaBLE::HandleWriteFrameRaw(data, len)) return;

      // Debug: Print received data in HEX format
      Serial.print("Received HEX: ");
      for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) {
          Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Process command if the first byte is 0x03
      if (data[0] == 0x03) {
        // Tare command: second byte 0x0F
        if (data[1] == 0x0F) {
          if (validateChecksum(data, len)) {
            Serial.println("Valid checksum for tare operation.");
          } else {
            Serial.println("Invalid checksum for tare operation.");
          }
          xTaskCreate( // To prevent halting the loop
          [] (void * parameter) {
          tareScale(); // Tare the scale
          vTaskDelete(NULL); // Delete the task once done
          },
          "TareTask", // Task name
          10000, // Stack size
          NULL, // Task parameter
          1, // Task priority
          NULL // Task handle
          );
        }
        // Power command: second byte 0x0A (Power off)
        else if (data[1] == 0x0A) {
          if (data[2] == 0x02) {
            Serial.println("Power off detected.");
            deep_sleep();
          }
        }
        //Calibration commands: second byte 0x1A
        else if (data[1] == 0x1A) {
          if (data[2] == 0x00) {
            Serial.println("Calibration 100g reference");
            bool success = doCalibration(100); // This will only calibrate properly if necessary preconditions are met
            if (success) {
              sendBleTask(1); // Success message
            } else {
              sendBleTask(0); // Failure message
            }
          }
          else if (data[2] == 0x01) {
            Serial.println("Calibration 200g reference");
            bool success = doCalibration(200);
            if (success) {
              sendBleTask(1); // Success message
            } else {
              sendBleTask(0); // Failure message
            }
          }
          else if (data[2] == 0x02) {
            Serial.println("Calibration 50g reference");
            bool success = doCalibration(50);
            if (success) {
              sendBleTask(1); // Success message
            } else {
              sendBleTask(0); // Failure message
            }
          }
          else {
            Serial.println("Unknown calibration command.");
          }
        }
        // Timer commands: second byte 0x0B
        else if (data[1] == 0x0B) {
          if (data[2] == 0x03) {
            Serial.println("Timer start detected.");
            timer_running = true;
            last_update = millis();
          } else if (data[2] == 0x00) {
            Serial.println("Timer stop detected.");
            timer_running = false;
          } else if (data[2] == 0x02) {
            Serial.println("Timer reset detected.");
            timer = 0;
            timer_running = false;
          }
        }
        else if (data[1] == 0x1B) {
          if (data[2] == 0x01) {
            Serial.println("Check FW version command received.");
            splitVersionString(FW_VERSION, version, subversion, patch);
            sendFWVersion(version, subversion, patch); // Send FW version response
          }
        }
        else if (data[1] == 0x22) {
          float currentOffset = getOffset();
          sendBleOffset(currentOffset); // Send current offset
          Serial.print("Sent current offset: ");
          Serial.println(currentOffset);
        }
        else if (data[1] == 0x44) {
          float uvalue = decodeOffset(data[2], data[3], data[4]);
          bool check = setOffset(uvalue);
          if (check) {
            Serial.print("Offset set successfully: ");
            Serial.println(uvalue);
            sendBleTask(3); // Success message
          } else {
            Serial.println("Failed to set offset.");
            sendBleTask(0); // Failure message
          }
        }
      }
    }
  }
};

// Send weight via BLE notification
void sendBleWeight() {
  if (deviceConnected && !EspressiOtaBLE::IsActive()) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastWeightNotifyTime >= weightNotifyInterval) {
      lastWeightNotifyTime = currentMillis;
      byte data[7];
      int voltage = batteryStatus * 10;  // Get the current battery voltage
      float weight = currentWeight;
      byte weightByte1, weightByte2;
      encodeWeight(weight, weightByte1, weightByte2);

      data[0] = modelByte;
      data[1] = 0xCE;  // Type byte for weight stable
      data[2] = weightByte1;
      data[3] = weightByte2;
      data[4] = voltage;
      data[5] = 0x00;
      data[6] = calculateXOR(data, 6);  // Calculate checksum
      
      pReadCharacteristic->setValue(data, 7);
      pReadCharacteristic->notify();
      Serial.print("Notified weight: ");
      Serial.println(weight);
      Serial.print("Battery voltage: ");
      Serial.println(voltage / 10.0);
    }
  }
}

// ============================
// BLE Setup task
// ============================
void setupBLE(void * parameter) {
  BLEDevice::init("EspressiScale"); // Initialize BLE with device name
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SUUID_ESPRESSISCALE);
  Serial.print("BLE service created");
  Serial.print("Service UUID: ");
  Serial.println(SUUID_ESPRESSISCALE);

  // Create BLE Write Characteristic
  pWriteCharacteristic = pService->createCharacteristic(
      CUUID_ESPRESSISCALE_WRITE,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pWriteCharacteristic->setCallbacks(new MyCallbacks());

  // Create BLE Read/Notify Characteristic
  pReadCharacteristic = pService->createCharacteristic(
      CUUID_ESPRESSISCALE_READ,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pReadCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  esp_ble_gatt_set_local_mtu(517);
  Serial.println("BLE advertising started");
  vTaskDelete(NULL); // Delete this task after setup
}

void setup()
{
  for (auto pin : holdPins) {
    gpio_hold_dis(pin);
  }

  touch_eg = xEventGroupCreate();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 0); // Touch interrupt is connected to GPIO 12

  Serial.begin(921600);
  Serial.println("HX711 with median filter and exponential smoothing");
  jd9613_init();
  TFT_CS_0_L;
  lcd_PushColors(0, 0, 294, 126, (uint16_t *)espressiscale_right_map, 1);
  TFT_CS_0_H;
  TFT_CS_1_L;
  lcd_PushColors(0, 0, 294, 126, (uint16_t *)espressiscale_left_map, 3);
  TFT_CS_1_H;
  delay(3000);

  lv_init();

  buf = (lv_color_t *)ps_malloc(lv_buffer_size);

  assert(buf);

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  /*Set the resolution of the display*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.full_refresh = 1;

  lv_disp_drv_register(&disp_drv);

  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
  touch.init();
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lv_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  setupScale();
  setupBattery();

  // Clear the display after showing the logo
  lv_obj_clean(lv_scr_act());

  // Set the background color to black
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);

  // Create a label to display the weight
  label_weight = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_weight, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_obj_align(label_weight, LV_ALIGN_RIGHT_MID, -10, 0);

  // Create a label to display the timer
  label_timer = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_obj_align(label_timer, LV_ALIGN_LEFT_MID, 10, 0); // Align to the left

  label_battery = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(label_battery, &lv_font_montserrat_28, LV_PART_MAIN);
  lv_obj_align(label_battery, LV_ALIGN_TOP_RIGHT, -8, 6);
  lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_FULL);

  flow_label = lv_label_create(lv_scr_act());   // Label for flow dots
  lv_obj_set_width(flow_label, FLOW_WIDTH);
  lv_obj_set_style_text_font(flow_label, &lv_font_montserrat_48, LV_PART_MAIN);
  lv_obj_set_style_text_align(flow_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_label_set_long_mode(flow_label, LV_LABEL_LONG_CLIP);


  lv_obj_align(flow_label, LV_ALIGN_BOTTOM_LEFT, 0, -4);

 
  lv_label_set_text(flow_label, "");

  xTaskCreatePinnedToCore(
    startWifi, // Function to run on this task
    "startWifi", // Task name
    10000, // Stack size
    NULL, // Task parameter
    1, // Task priority
    NULL, // Task handle
    0 // Task core
  );
  xTaskCreatePinnedToCore(
    setupBLE, // Function to run on this task
    "setupBLE", // Task name
    10000, // Stack size
    NULL, // Task parameter
    1, // Task priority
    NULL, // Task handle
    0 // Task core
  );
  tareScale(); // Tare the scale at startup
}

void loop()
{
  server.handleClient();
  // Read filtered weight
  currentWeight = medianFilter();
  sendBleWeight(); // Send weight via BLE notification
  getFlow(); // Update flow dots based on weight change

  // Update the label with the current weight
  char weight_str[16];
  snprintf(weight_str, sizeof(weight_str), "%.1f g", currentWeight);
  lv_label_set_text(label_weight, weight_str);

  if (touch.read())
  {
    TP_Point t = touch.getPoint(0);
    int16_t x = t.y; // Adjusted to match the screen orientation

    if (x > screenWidth / 2 && !prevTouched)
    {
      timer_running = !timer_running; // Toggle timer state
      delay(100); // Debounce delay
    }
    else if (x <= screenWidth / 2 && !prevTouched)
    {
      timer_running = false; // Stop the timer
      timer = 0; // Reset timer
      xTaskCreate( // To prevent halting the loop
        [] (void * parameter) {
          bool ok = tareScale();
          if (ok) {
            Serial.println("Tare successful");
          } else {
            Serial.println("Tare failed");
          }
          vTaskDelete(NULL); // Delete the task once done
        },
        "TareTask", // Task name
        10000, // Stack size
        NULL, // Task parameter
        1, // Task priority
        NULL // Task handle
      );
      Serial.println("Tared and timer reset via touch");
    }
    if (!prevTouched) {
      touchStart = millis();
    }

    // If touch is still held and >1000ms
    if (millis() - touchStart > 1000) {
      Serial.println("Long press detected");
      lv_task_handler(); // Ensure LVGL updates the display
      lv_label_set_text(label_weight, "Deep Sleep");
      delay(2000); // Wait for 2 seconds to show the message
      if (!touch.read()) {
        Serial.println("Entering deep sleep");
        deep_sleep();  // entering deep sleep
      }
    }

    prevTouched = true;
  }
  else {
    prevTouched = false; // Reset touch state
  }

  if (timer_running)
  {
    unsigned long current_time = millis();
    if (current_time - last_update >= 1000) // Update every second
    {
      timer++;
      last_update = current_time;
    }
  }

  // Update the label with the timer value
  char timer_str[16];
  snprintf(timer_str, sizeof(timer_str), "%d s", timer);
  lv_label_set_text(label_timer, timer_str);

  // Handle LittlevGL tasks
  lv_task_handler();
  delay(5);
  static unsigned long last_activity_time = 0; // Last activity time
  static float lastWeight = currentWeight; // Last weight value

  // Check if the timer is not running and weight hasn't changed significantly
  if (!timer_running && abs(currentWeight - lastWeight) < 1.0 && !EspressiOtaBLE::IsActive())
  {
    unsigned long current_time = millis();
    if (current_time - last_activity_time >= 300000) // 5 minutes
    {
      Serial.println("Entering deep sleep due to inactivity...");
      // Flush the screen to black before going to deep sleep
      deep_sleep();
    }
  }
  else
  {
    last_activity_time = millis(); // Reset the activity timer
  }

  lastWeight = currentWeight; // Update the last weight value

  static unsigned long lastBatteryCheck = -60000;
  if (millis() - lastBatteryCheck >= 60000 && !EspressiOtaBLE::IsActive() && !deviceConnected) { // Check every 1 minute
    batteryStatus = getBatteryVoltage(); // Update the battery status
    lastBatteryCheck = millis();
    updateBatteryIcon(batteryStatus); // Update the battery icon on the display
    Serial.print("Battery voltage: ");
    Serial.println(batteryStatus);
  }
  
  if (batteryStatus < 3) // Check if battery voltage is below 3V
  {
    Serial.println("Battery voltage is low. Entering deep sleep...");
    // Display low battery message before going to deep sleep
    lv_label_set_text(label_weight, "Low battery");
    delay(2000); // Wait for 2 seconds to show the message
    deep_sleep();
  }
}