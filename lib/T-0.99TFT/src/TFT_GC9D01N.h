
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <SPI.h>

// #define LANDSCAPE 1   //Horizontal screen
#define PORTRAIT 2
#define CHAR_FONT_W8_H16  //typeface

#define TFT_WIDTH  40
#define TFT_HEIGHT 160


// #define TFT_MISO  -1//41//不知道为什么  设置 -1 io13不能设置高低电平  需要设置SPI后再把io13做为输出才正常
// #define TFT_MOSI  18
// #define TFT_SCLK  17
// // #define TFT_DC  15 // Data Command control pin
// #define TFT_DC    16 // Data Command control pin
// #define TFT_RST   14
// #define TFT_CS    -1//13//11  // Chip select control pin
// #define TFT_BL    -1//48


#define TFT_MISO  -1//41//不知道为什么  设置 -1 io13不能设置高低电平  需要设置SPI后再把io13做为输出才正常
#define TFT_MOSI  18
#define TFT_SCLK  17
// #define TFT_DC  15 // Data Command control pin
#define TFT_DC    16 // Data Command control pin
#define TFT_RST   15
#define TFT_CS    -1//13//11  // Chip select control pin
#define TFT1_CS 14
#define TFT2_CS 13
#define TFT_BL   48

//RGB565
#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define WHITE  0xFFFF
#define BLACK  0x0000
#define GRAY   0xEF5D
#define GRAY75 0x39E7
#define GRAY50 0x7BEF
#define GRAY25 0xADB5



#ifdef  CHAR_FONT_W8_H16
#define  FONT_W  8
#define  FONT_H  16
#endif


#define SPI_FREQUENCY  40000000
#define DC_C digitalWrite(TFT_DC, LOW)
#define DC_D digitalWrite(TFT_DC, HIGH)
#if (TFT_CS==-1)
#define CS_L ;//digitalWrite(TFT_CS, LOW)
#define CS_H ;//digitalWrite(TFT_CS, HIGH)
#else
#define CS_L digitalWrite(TFT_CS, LOW)
#define CS_H digitalWrite(TFT_CS, HIGH)
#endif

#if (TFT_RST==-1)
#define RST_L ;//digitalWrite(TFT_RST, LOW)
#define RST_H ;//digitalWrite(TFT_RST, HIGH)
#else
#define RST_L digitalWrite(TFT_RST, LOW)
#define RST_H digitalWrite(TFT_RST, HIGH)
#endif


class TFT_GC9D01N_Class
{
public:

    int begin();
    void begin_tft_write();
    void end_tft_write();
    void writedata(uint8_t d);
    void writecommand(uint8_t c);
    void lcd_init();
    void backlight(uint8_t state);
    void BlockWrite(unsigned int Xstart, unsigned int Xend, unsigned int Ystart, unsigned int Yend) ;
    void DrawImage(unsigned int Xstart, unsigned int Ystart, unsigned int Xend, unsigned int Yend, uint16_t *color);
    void DispColor(unsigned int Xstart, unsigned int Ystart, unsigned int Xend, unsigned int Yend, unsigned int color);
    void WriteOneDot(unsigned int color);
    void  DispOneChar( char ord, unsigned int Xstart, unsigned int Ystart, unsigned int TextColor, unsigned int BackColor);
    void DispStr( char *str, unsigned int Xstart, unsigned int Ystart, unsigned int TextColor, unsigned int BackColor);

};
