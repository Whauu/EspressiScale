#include "TFT_eSPI.h"
// This is the command sequence that initialises the ST7789 driver
//
// This setup information uses simple 8 bit SPI writecommand() and writedata() functions
//
// See ST7735_Setup.h file for an alternative format

// T-Display_Bar ST7789
#ifndef INIT_SEQUENCE_3
{
  writecommand(ST7789_SLPOUT); // Sleep out
  delay(120);

  writecommand(ST7789_PORCTRL); // Normal display mode on
  writedata(0x0C);
  writedata(0x0C);
  writedata(0x00);
  writedata(0x33);
  writedata(0x33);

  writecommand(ST7789_RAMCTRL);
  writedata(0x00);
  writedata(0xE0);
  //------------------------------display and color format setting--------------------------------//
  // writecommand(ST7789_COLMOD);
  // writedata(0x55); //0x55  // 16bit RGB 565
  // delay(10);

  writecommand(ST7789_MADCTL);
  writedata(0x00);

  writecommand(TFT_COLMOD);
  writedata(0x05);

  writecommand(ST7789_GCTRL);
  writedata(0x45);

  writecommand(ST7789_VCOMS);
  writedata(0x1D);

  writecommand(ST7789_LCMCTRL);
  writedata(0x2C);

  writecommand(ST7789_VDVVRHEN);
  writedata(0x01);

  writecommand(ST7789_VRHS);
  writedata(0x19);

  writecommand(ST7789_VDVSET);
  writedata(0x20);

  writecommand(ST7789_FRCTR2);
  writedata(0x0F);

  writecommand(ST7789_PWCTRL1);
  writedata(0xA4);
  writedata(0xA1);

  writecommand(0xD6);
  writedata(0xA1);

  writecommand(ST7789_PVGAMCTRL);
  writedata(0xD0);
  writedata(0x10);
  writedata(0x21);
  writedata(0x14);
  writedata(0x15);
  writedata(0x2D);
  writedata(0x41);
  writedata(0x44);
  writedata(0x4F);
  writedata(0x28);
  writedata(0x0E);
  writedata(0x0C);
  writedata(0x1D);
  writedata(0x1F);

  writecommand(ST7789_NVGAMCTRL);
  writedata(0xD0);
  writedata(0x0F);
  writedata(0x1B);
  writedata(0x0D);
  writedata(0x0D);
  writedata(0x26);
  writedata(0x42);
  writedata(0x54);
  writedata(0x50);
  writedata(0x3E);
  writedata(0x1A);
  writedata(0x18);
  writedata(0x22);
  writedata(0x25);
  writecommand(ST7789_DISPON);

  writecommand(ST7789_CASET);
  writedata(0x00);
  writedata(0x52);
  writedata(0x00);
  writedata(0x9D);

  writecommand(ST7789_RASET);
  writedata(0x00);
  writedata(0x12);
  writedata(0x01);
  writedata(0x2D);

  writecommand(ST7789_RAMWR);
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  delay(120);
  end_tft_write();
  
}
#else
// TTGO ESP32 S3 T-Display
{
  writecommand(ST7789_SLPOUT); // Sleep out
  delay(120);

  writecommand(ST7789_NORON); // Normal display mode on

  //------------------------------display and color format setting--------------------------------//
  writecommand(ST7789_MADCTL);
  writedata(TFT_MAD_COLOR_ORDER);

  // writecommand(ST7789_RAMCTRL);
  // writedata(0x00);
  // writedata(0xE0); // 5 to 6 bit conversion: r0 = r5, b0 = b5

  writecommand(ST7789_COLMOD);
  writedata(0x55);
  delay(10);

  //--------------------------------ST7789V Frame rate setting----------------------------------//
  writecommand(ST7789_PORCTRL);
  writedata(0x0b);
  writedata(0x0b);
  writedata(0x00);
  writedata(0x33);
  writedata(0x33);

  writecommand(ST7789_GCTRL); // Voltages: VGH / VGL
  writedata(0x75);

  //---------------------------------ST7789V Power setting--------------------------------------//
  writecommand(ST7789_VCOMS);
  writedata(0x28); // JLX240 display datasheet

  writecommand(ST7789_LCMCTRL);
  writedata(0x2C);

  writecommand(ST7789_VDVVRHEN);
  writedata(0x01);

  writecommand(ST7789_VRHS); // voltage VRHS
  writedata(0x1F);

  writecommand(ST7789_FRCTR2);
  writedata(0x13);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa7);

  writecommand(ST7789_PWCTRL1);
  writedata(0xa4);
  writedata(0xa1);

  writecommand(0xD6);
  writedata(0xa1);

  //--------------------------------ST7789V gamma setting---------------------------------------//
  writecommand(ST7789_PVGAMCTRL);
  writedata(0xf0);
  writedata(0x05);
  writedata(0x0a);
  writedata(0x06);
  writedata(0x06);
  writedata(0x03);
  writedata(0x2b);
  writedata(0x32);
  writedata(0x43);
  writedata(0x36);
  writedata(0x11);
  writedata(0x10);
  writedata(0x2b);
  writedata(0x32);

  writecommand(ST7789_NVGAMCTRL);
  writedata(0xf0);
  writedata(0x08);
  writedata(0x0c);
  writedata(0x0b);
  writedata(0x09);
  writedata(0x24);
  writedata(0x2b);
  writedata(0x22);
  writedata(0x43);
  writedata(0x38);
  writedata(0x15);
  writedata(0x16);
  writedata(0x2f);
  writedata(0x37);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  end_tft_write();
  delay(120);
  begin_tft_write();

  writecommand(ST7789_DISPON); // Display on
  delay(120);

#ifdef TFT_BL
  // Turn on the back-light LED
  digitalWrite(TFT_BL, HIGH);
  pinMode(TFT_BL, OUTPUT);
#endif
}
#endif