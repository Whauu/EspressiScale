#include "arduino.h"
#include <battery.h>
#include <scale.h>
#include <filter.h>
#include "jd9613.h"
#include "lvgl.h"
#include "pin_config.h"
#include "SPI.h"
#include "time.h"
#include "esp_sntp.h"
#define TOUCH_MODULES_CST_SELF
#include "TouchLib.h"
#include "Wire.h"
#include <BLE.h>
#include <WiFi.hpp>

#ifndef BOARD_HAS_PSRAM
#error "Please turn on PSRAM option to OPI PSRAM"
#endif

static const uint16_t screenWidth = 294 * 2; // screenWidth = 294 * 2;
static const uint16_t screenHeight = 126;
static const size_t lv_buffer_size = screenWidth * screenHeight * sizeof(lv_color_t);
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf = NULL;
lv_obj_t *label_weight = NULL;
lv_obj_t *label_timer = NULL; // New label for timer

static int timer = 0; // Initialize timer to 0
static bool timer_running = false; // Timer running state
static unsigned long last_update = 0; // Last update time
float currentWeight = 0.0; // Current weight

static EventGroupHandle_t touch_eg;
#define GET_TOUCH_INT _BV(1)

extern uint8_t espressiscale_left_map[];
extern uint8_t espressiscale_right_map[];

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
  esp_wifi_stop();
  esp_deep_sleep_start();
}

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

// Helper function to calculate XOR checksum for outgoing data
uint8_t calculateXOR(uint8_t *data, size_t len) {
  uint8_t xorValue = 0x03; // Starting value for XOR as per your protocol
  for (size_t i = 1; i < len - 1; i++) { // Start at index 1; reserve last byte for checksum
    xorValue ^= data[i];
  }
  return xorValue;
}

// Helper function to encode weight into two bytes
void encodeWeight(float weight, byte &byte1, byte &byte2) {
  int weightInt = (int)(weight * 10);  // Convert to an integer (weight in grams * 10)
  byte1 = (byte)((weightInt >> 8) & 0xFF);
  byte2 = (byte)(weightInt & 0xFF);
}

// BLE Server Callbacks
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

  void onWrite(BLECharacteristic *pWriteCharacteristic) {
    if (pWriteCharacteristic != nullptr) {
      size_t len = pWriteCharacteristic->getLength();
      uint8_t *data = (uint8_t *)pWriteCharacteristic->getData();

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
          tareScale();  // Call the external tare function
        }
        // LED commands: second byte 0x0A (only LED on/off are processed)
        else if (data[1] == 0x0A) {
          if (data[2] == 0x00) {
            Serial.println("LED off detected.");
          } else if (data[2] == 0x01) {
            Serial.println("LED on detected.");
          }
          // Power down branch removed.
        }
        // Timer commands: second byte 0x0B
        else if (data[1] == 0x0B) {
          if (data[2] == 0x03) {
            Serial.println("Timer start detected.");
            timer_running = true;
          } else if (data[2] == 0x00) {
            Serial.println("Timer stop detected.");
            timer_running = false;
          } else if (data[2] == 0x02) {
            Serial.println("Timer reset detected.");
            timer = 0;
          }
        }
      }
    }
  }
};

// Function to send weight via BLE notification
void sendBleWeight() {
  if (deviceConnected) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastWeightNotifyTime >= weightNotifyInterval) {
      lastWeightNotifyTime = currentMillis;
      byte data[7];
      float weight = currentWeight;
      byte weightByte1, weightByte2;
      encodeWeight(weight, weightByte1, weightByte2);
      
      data[0] = modelByte;
      data[1] = 0xCE;  // Type byte for weight stable
      data[2] = weightByte1;
      data[3] = weightByte2;
      data[4] = 0x00;
      data[5] = 0x00;
      data[6] = calculateXOR(data, 6);  // Calculate checksum
      
      pReadCharacteristic->setValue(data, 7);
      pReadCharacteristic->notify();
      Serial.print("Notified weight: ");
      Serial.println(weight);
    }
  }
}

void setupBLE(void * parameter) {
  BLEDevice::init("EspressiScale"); // Initialize BLE with device name
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SUUID_DECENTSCALE);

  // Create BLE Write Characteristic
  pWriteCharacteristic = pService->createCharacteristic(
      CUUID_DECENTSCALE_WRITE,
      BLECharacteristic::PROPERTY_WRITE);
  pWriteCharacteristic->setCallbacks(new MyCallbacks());

  // Create BLE Read/Notify Characteristic
  pReadCharacteristic = pService->createCharacteristic(
      CUUID_DECENTSCALE_READ,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pReadCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("BLE advertising started");
  vTaskDelete(NULL); // Delete this task after setup
}

void setup()
{
  for (auto pin : holdPins) {
    gpio_hold_dis(pin);
  }
  setupCalibrationFactor();
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
}

void loop()
{
  server.handleClient();
  // Read filtered weight
  currentWeight = medianFilter();
  sendBleWeight(); // Send weight via BLE notification

  // Update the label with the current weight
  char weight_str[16];
  snprintf(weight_str, sizeof(weight_str), "%.1f g", currentWeight);
  lv_label_set_text(label_weight, weight_str);

  if (touch.read())
  {
    TP_Point t = touch.getPoint(0);
    int16_t x = t.y; // Adjusted to match the screen orientation

    if (x > screenWidth / 2)
    {
      timer_running = !timer_running; // Toggle timer state
      delay(1000); // Debounce delay
    }
    else if (x < screenWidth / 2 && x > screenWidth / 2)
    {
      Serial.println("Touch registered on both sides, entering deep sleep...");
      deep_sleep(); // Enter deep sleep if both sides are touched
    }
    else
    {
      timer_running = false; // Stop the timer
      timer = 0; // Reset timer
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
      Serial.println("Tared and timer reset via touch");
    }
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
  if (!timer_running && abs(currentWeight - lastWeight) < 1.0)
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

  float batteryStatus = getBatteryVoltage(); // Update the battery status
  
  if (batteryStatus < 2.8) // Check if battery voltage is below 3V
  {
    Serial.println("Battery voltage is low. Entering deep sleep...");
    // Display low battery message before going to deep sleep
    lv_label_set_text(label_weight, "Low battery");
    delay(2000); // Wait for 2 seconds to show the message
    deep_sleep();
  }
}