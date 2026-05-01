#pragma once
#define LV_DELAY(x)                                                                                                                                  \
  do {                                                                                                                                               \
    uint32_t t = x;                                                                                                                                  \
    while (t--) {                                                                                                                                    \
      lv_timer_handler();                                                                                                                            \
      delay(1);                                                                                                                                      \
    }                                                                                                                                                \
  } while (0);

#define TFT_DC        16
#define TFT_RES       15
#define TFT_CS_0      13
#define TFT_CS_1      14
#define TFT_MOSI      18
#define TFT_SCK       17


#define PIN_IIC_SCL   11
#define PIN_IIC_SDA   10
#define PIN_TOUCH_INT 12
#define PIN_TOUCH_RES 9