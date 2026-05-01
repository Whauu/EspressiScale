#include "jd9613.h"
#include "SPI.h"
#include "Arduino.h"
#include "pin_config.h"

uint8_t horizontal = 0;

extern void my_print(const char *buf);

typedef struct
{
    uint8_t  addr;
    uint8_t  param[16];
    uint32_t len;
} lcd_cmd_t;
// clang-format off
 static const lcd_cmd_t JD9613_CMD[] = {
    {0xfe, {0x01}, 0x02},
    {0xf7, {0x96, 0x13, 0xa9}, 0x04},
    {0x90, {0x01}, 0x02},
    {0x2c, {0x19, 0x0b, 0x24, 0x1b, 0x1b, 0x1b, 0xaa, 0x50, 0x01, 0x16, 0x04, 0x04, 0x04, 0xd7}, 0x0f},
    {0x2d, {0x66, 0x56, 0x55}, 0x04},
    {0x2e, {0x24, 0x04, 0x3f, 0x30, 0x30, 0xa8, 0xb8, 0xb8, 0x07}, 0x0a},
    {0x33, {0x03, 0x03, 0x03, 0x19, 0x19, 0x19, 0x13, 0x13, 0x13, 0x1a, 0x1a, 0x1a}, 0x0d},
    {0x10, {0x0b, 0x08, 0x64, 0xae, 0x0b, 0x08, 0x64, 0xae, 0x00, 0x80, 0x00, 0x00, 0x01}, 0x0e},
    {0x11, {0x01, 0x1e, 0x01, 0x1e, 0x00}, 0x06},
    {0x03, {0x93, 0x1c, 0x00, 0x01, 0x7e}, 0x06},
    {0x19, {0x00}, 0x02},
    {0x31, {0x1b, 0x00, 0x06, 0x05, 0x05, 0x05}, 0x07},
    {0x35, {0x00, 0x80, 0x80, 0x00}, 0x05},
    {0x12, {0x1b}, 0x02},
    {0x1a, {0x01, 0x20, 0x00, 0x08, 0x01, 0x06, 0x06, 0x06}, 0x09},
    {0x74, {0xbd, 0x00, 0x01, 0x08, 0x01, 0xbb, 0x98}, 0x08},
    {0x6c, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x01, 0x30, 0x08, 0x00}, 0x0a},
    {0x6d, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x02, 0x30, 0x08, 0x00}, 0x0a},
    {0x76, {0xda, 0x00, 0x02, 0x20, 0x39, 0x80, 0x80, 0x50, 0x05}, 0x0a},
    {0x6e, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x02, 0x4f, 0x02, 0x00}, 0x0a},
    {0x6f, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x01, 0x4f, 0x02, 0x00}, 0x0a},
    {0x80, {0xbd, 0x00, 0x01, 0x08, 0x01, 0xbb, 0x98}, 0x08},
    {0x78, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x01, 0x30, 0x08, 0x00}, 0x0a},
    {0x79, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x02, 0x30, 0x08, 0x00}, 0x0a},
    {0x82, {0xda, 0x40, 0x02, 0x20, 0x39, 0x00, 0x80, 0x50, 0x05}, 0x0a},
    {0x7a, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x02, 0x4f, 0x02, 0x00}, 0x0a},
    {0x7b, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x01, 0x4f, 0x02, 0x00}, 0x0a},
    {0x84, {0x01, 0x00, 0x09, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19}, 0x0b},
    {0x85, {0x19, 0x19, 0x19, 0x03, 0x02, 0x08, 0x19, 0x19, 0x19, 0x19}, 0x0b},
    {0x20, {0x20, 0x00, 0x08, 0x00, 0x02, 0x00, 0x40, 0x00, 0x10, 0x00, 0x04, 0x00}, 0x0d},
    {0x1e, {0x40, 0x00, 0x10, 0x00, 0x04, 0x00, 0x20, 0x00, 0x08, 0x00, 0x02, 0x00}, 0x0d},
    {0x24, {0x20, 0x00, 0x08, 0x00, 0x02, 0x00, 0x40, 0x00, 0x10, 0x00, 0x04, 0x00}, 0x0d},
    {0x22, {0x40, 0x00, 0x10, 0x00, 0x04, 0x00, 0x20, 0x00, 0x08, 0x00, 0x02, 0x00}, 0x0d},
    {0x13, {0x63, 0x52, 0x41}, 0x04},
    {0x14, {0x36, 0x25, 0x14}, 0x04},
    {0x15, {0x63, 0x52, 0x41}, 0x04},
    {0x16, {0x36, 0x25, 0x14}, 0x04},
    {0x1d, {0x10, 0x00, 0x00}, 0x04},
    {0x2a, {0x0d, 0x07}, 0x03},
    {0x27, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}, 0x07},
    {0x28, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}, 0x07},
    {0x26, {0x01, 0x01}, 0x03},
    {0x86, {0x01, 0x01}, 0x03},
    {0xfe, {0x02}, 0x02},
    {0x16, {0x81, 0x43, 0x23, 0x1e, 0x03}, 0x06},
    {0xfe, {0x03}, 0x02},
    {0x60, {0x01}, 0x02},
    {0x61, {0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x0d, 0x26, 0x5a, 0x80, 0x80, 0x95, 0xf8, 0x3b, 0x75}, 0x10},
    {0x62, {0x21, 0x22, 0x32, 0x43, 0x44, 0xd7, 0x0a, 0x59, 0xa1, 0xe1, 0x52, 0xb7, 0x11, 0x64, 0xb1}, 0x10},
    {0x63, {0x54, 0x55, 0x66, 0x06, 0xfb, 0x3f, 0x81, 0xc6, 0x06, 0x45, 0x83}, 0x0c},
    {0x64, {0x00, 0x00, 0x11, 0x11, 0x21, 0x00, 0x23, 0x6a, 0xf8, 0x63, 0x67, 0x70, 0xa5, 0xdc, 0x02}, 0x10},
    {0x65, {0x22, 0x22, 0x32, 0x43, 0x44, 0x24, 0x44, 0x82, 0xc1, 0xf8, 0x61, 0xbf, 0x13, 0x62, 0xad}, 0x10},
    {0x66, {0x54, 0x55, 0x65, 0x06, 0xf5, 0x37, 0x76, 0xb8, 0xf5, 0x31, 0x6c}, 0x0c},
    {0x67, {0x00, 0x10, 0x22, 0x22, 0x22, 0x00, 0x37, 0xa4, 0x7e, 0x22, 0x25, 0x2c, 0x4c, 0x72, 0x9a}, 0x10},
    {0x68, {0x22, 0x33, 0x43, 0x44, 0x55, 0xc1, 0xe5, 0x2d, 0x6f, 0xaf, 0x23, 0x8f, 0xf3, 0x50, 0xa6}, 0x10},
    {0x69, {0x65, 0x66, 0x77, 0x07, 0xfd, 0x4e, 0x9c, 0xed, 0x39, 0x86, 0xd3}, 0x0c},
    {0xfe, {0x05}, 0x02},
    {0x61, {0x00, 0x31, 0x44, 0x54, 0x55, 0x00, 0x92, 0xb5, 0x88, 0x19, 0x90, 0xe8, 0x3e, 0x71, 0xa5}, 0x10},
    {0x62, {0x55, 0x66, 0x76, 0x77, 0x88, 0xce, 0xf2, 0x32, 0x6e, 0xc4, 0x34, 0x8b, 0xd9, 0x2a, 0x7d}, 0x10},
    {0x63, {0x98, 0x99, 0xaa, 0x0a, 0xdc, 0x2e, 0x7d, 0xc3, 0x0d, 0x5b, 0x9e}, 0x0c},
    {0x64, {0x00, 0x31, 0x44, 0x54, 0x55, 0x00, 0xa2, 0xe5, 0xcd, 0x5c, 0x94, 0xcf, 0x09, 0x4a, 0x72}, 0x10},
    {0x65, {0x55, 0x65, 0x66, 0x77, 0x87, 0x9c, 0xc2, 0xff, 0x36, 0x6a, 0xec, 0x45, 0x91, 0xd8, 0x20}, 0x10},
    {0x66, {0x88, 0x98, 0x99, 0x0a, 0x68, 0xb0, 0xfb, 0x43, 0x8c, 0xd5, 0x0e}, 0x0c},
    {0x67, {0x00, 0x42, 0x55, 0x55, 0x55, 0x00, 0xcb, 0x62, 0xc5, 0x09, 0x44, 0x72, 0xa9, 0xd6, 0xfd}, 0x10},
    {0x68, {0x66, 0x66, 0x77, 0x87, 0x98, 0x21, 0x45, 0x96, 0xed, 0x29, 0x90, 0xee, 0x4b, 0xb1, 0x13}, 0x10},
    {0x69, {0x99, 0xaa, 0xba, 0x0b, 0x6a, 0xb8, 0x0d, 0x62, 0xb8, 0x0e, 0x54}, 0x0c},
    {0xfe, {0x07}, 0x02},
    {0x3e, {0x00}, 0x02},
    {0x42, {0x03, 0x10}, 0x03},
    {0x4a, {0x31}, 0x02},
    {0x5c, {0x01}, 0x02},
    {0x3c, {0x07, 0x00, 0x24, 0x04, 0x3f, 0xe2}, 0x07},
    {0x44, {0x03, 0x40, 0x3f, 0x02}, 0x05},
    {0x12, {0xaa, 0xaa, 0xc0, 0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xf0, 0xf8}, 0x0b},
    {0x11, {0xaa, 0xaa, 0xaa, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8}, 0x10},
    {0x10, {0xaa, 0xaa, 0xaa, 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58}, 0x10},
    {0x14, {0x03, 0x1f, 0x3f, 0x5f, 0x7f, 0x9f, 0xbf, 0xdf, 0x03, 0x1f, 0x3f, 0x5f, 0x7f, 0x9f, 0xbf, 0xdf}, 0x11},
    {0x18, {0x70, 0x1a, 0x22, 0xbb, 0xaa, 0xff, 0x24, 0x71, 0x0f, 0x01, 0x00, 0x03}, 0x0d},
    {0xfe, {0x00}, 0x02},
    {0x3a, {0x55}, 0x02},
    {0xc4, {0x80}, 0x02},
    {0x2a, {0x00, 0x00, 0x00, 0x7d}, 0x05},
    {0x2b, {0x00, 0x00, 0x01, 0x25}, 0x05},
    {0x35, {0x00}, 0x02},
    {0x53, {0x28}, 0x02},
    {0x51, {0xff}, 0x02},
    {0x11, {0x00}, 0x81},
    {0x29, {0x00}, 0x81}};
// clang-format on
static void tft_gpio_init(void)
{
    SPI.begin(TFT_SCK, -1, TFT_MOSI, -1);
    SPI.setFrequency(SPI_FREQUENCY);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_CS_0, OUTPUT);
    pinMode(TFT_CS_1, OUTPUT);
    pinMode(TFT_RES, OUTPUT);

    TFT_RES_L;
    delay(100);
    TFT_RES_H;
    delay(100);
}

static void WriteComm(uint8_t data)
{
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    TFT_DC_L;
    SPI.write(data);
    TFT_DC_H;
    SPI.endTransaction();
}

static void WriteData(uint8_t data)
{
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    TFT_DC_H;
    SPI.write(data);
    SPI.endTransaction();
}

static void WriteData16(uint16_t data)
{
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    TFT_DC_H;
    SPI.write16(data);
    SPI.endTransaction();
}

void jd9613_init(void)
{
    TFT_CS_0_L;
    TFT_CS_1_L;
    tft_gpio_init();

    const lcd_cmd_t *t = JD9613_CMD;
    for (uint32_t i = 0; i < (sizeof(JD9613_CMD) / sizeof(lcd_cmd_t)); i++)
    {
        WriteComm(t[i].addr);
        for (uint8_t j = 0; j < ((t[i].len & 0x7F) - 1); j++)
        {
            WriteData(t[i].param[j]);
        }
        if (t[i].len & 0x80) delay(120);
    }
    TFT_CS_0_H;
    TFT_CS_1_H;
}


void lcd_setRotation(uint8_t r)
{
    horizontal = r % 4;
    WriteComm(TFT_MADCTL);
    switch (horizontal)
    {
    case 0: // Portrait
        WriteData(TFT_MAD_BGR);
        break;
    case 1: // Landscape (Portrait + 90)
        WriteData(TFT_MAD_MX | TFT_MAD_MV | TFT_MAD_BGR);
        break;
    case 2: // Inverter portrait
        WriteData(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_BGR);
        break;
    case 3: // Inverted landscape
        WriteData(TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR);
        break;
    }
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    WriteComm(0x2a);
    WriteData16(x1);
    WriteData16(x2);
    WriteComm(0x2b);
    WriteData16(y1);
    WriteData16(y2);
    WriteComm(0x2c);
}

void lcd_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    //uint16_t i, j;
    uint32_t len = w * h;
    LCD_Address_Set(x, y, x + w - 1, y + h - 1);
    while (len--)
    {
        WriteData16(color);
    }
}


void lcd_PushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t high, uint16_t *data)
{
    LCD_Address_Set(x, y, x + width - 1, y + high - 1);
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    TFT_DC_H;
    // SPI.writeBytes((uint8_t *)data, width * high * 2);
    for (int i = 0; i < width * high; i++)
    {
        SPI.write16(data[i]);
    }

    SPI.endTransaction();
}

void lcd_PushColors(uint16_t *data, uint32_t len)
{
    SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
    TFT_DC_H;
    SPI.writeBytes((uint8_t *)data, len * 2);
    SPI.endTransaction();
}

void lcd_PushColors_SoftRotation(uint16_t  x,
                                 uint16_t  y,
                                 uint16_t  width,
                                 uint16_t  high,
                                 uint16_t *data,
                                 uint8_t   r)
{
    uint16_t  _x = 126 - (y + high);
    uint16_t  _y = x;
    uint16_t  _h = width;
    uint16_t  _w = high;
    uint16_t *p = data;

    LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);

    /*char str[40];
    sprintf(str, "spi: x =%d, y=%d; h =%d, w=%d, r=%d;\n", x, y, high, width, r);
    my_print(str);*/


    if(r == 2)
    {
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 294-x; j < width+294-x; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[(width+294-x) * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]>>8);
            }
        }    
    }
    if(r == 1)
    {
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        /*for (uint16_t j = 0; j+x < 294; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[(width+294) * (high - i - 1) + j]);
                SPI.write16((uint16_t)p[(126*294*2)-294-((width+294) * (high - i - 1) + j)]);
            }
        }    */
        for (uint16_t j = 0; j+x < 294; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[(126*294*2)-294-((width+294) * (high - i - 1) + j)]);
            }
        }
    }

    SPI.endTransaction();
}

/*
void lcd_PushColors(uint16_t  x,
                    uint16_t  y,
                    uint16_t  width,
                    uint16_t  high,
                    uint16_t *data,
                    uint8_t   rotation)
{
    if (rotation == 0)
    {
        uint16_t  _x = 126 - (y + high);
        uint16_t  _y = x;
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        //lcd_PushColors(x, y, width, high, data);
        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);

        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]>>8);
            }
        }

        SPI.endTransaction();
    }
    else if (rotation == 1)
    {
        uint16_t  _x = TFT_WIDTH - (y + high);
        uint16_t  _y = x;
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
            }
        }
        SPI.endTransaction();
    }
    else if (rotation == 2)
    {
        // LCD_Address_Set(x, 294 - (y + high - 1), x + width - 1, 294 - y - 1);

        // SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        // TFT_DC_H;

        // for (int i = (width * high); i > 0; i--)
        // {
        //     SPI.write16(data[i]);
        // }

        // SPI.endTransaction();

            uint16_t  _x = 126 - (y + high);
        uint16_t  _y = x;
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);

       // TFT_CS_L;
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]>>8);
            }
        }

        SPI.endTransaction();

    }
    else if (rotation == 3)
    {
        uint16_t  _x = y;
        uint16_t  _y = TFT_HEIGHT - (x + width);
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);

        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (i + 1) - j - 1]);
            }
        }
        SPI.endTransaction();
    }
}
*/

void lcd_PushColors(uint16_t  x,
                    uint16_t  y,
                    uint16_t  width,
                    uint16_t  high,
                    uint16_t *data,
                    uint8_t   rotation)
{
    uint16_t *p = data;
    if (rotation == 0)
    {
        //lcd_PushColors(x, y, width, high, data);
        LCD_Address_Set(x, y, x + width - 1, y + high - 1);

        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
       /* for (uint16_t i = 0; i < high; i++)
        {
            for (uint16_t j = 0; j < width; j++)
            {
                 SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]>>8);
            }
        }*/

        char str[40];
        sprintf(str, "spi: x =%d, y=%d;\n", high, width);
        my_print(str);


        for (uint16_t i = 0; i < high; i++)
        {
            for (uint16_t j = 0; j < width; j++)
            {
                 SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]);
                // SPI.write(p[width * (high - i - 1) + j]>>8);
            }
        }


        SPI.endTransaction();
    }
    else if (rotation == 1)
    {
        uint16_t  _x = TFT_WIDTH - (y + high);
        uint16_t  _y = x;
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);
        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (high - i - 1) + j]);
            }
        }
        SPI.endTransaction();
    }
    else if (rotation == 2)
    {
        LCD_Address_Set(x, 294 - (y + high - 1), x + width - 1, 294 - y - 1);

        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;

        for (int i = (width * high); i > 0; i--)
        {
            SPI.write16(data[i]);
        }

        SPI.endTransaction();
    }
    else if (rotation == 3)
    {
        uint16_t  _x = y;
        uint16_t  _y = TFT_HEIGHT - (x + width);
        uint16_t  _h = width;
        uint16_t  _w = high;
        uint16_t *p = data;

        LCD_Address_Set(_x, _y, _x + _w - 1, _y + _h - 1);

        SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
        TFT_DC_H;
        for (uint16_t j = 0; j < width; j++)
        {
            for (uint16_t i = 0; i < high; i++)
            {
                SPI.write16((uint16_t)p[width * (i + 1) - j - 1]);
            }
        }
        SPI.endTransaction();
    }
}
