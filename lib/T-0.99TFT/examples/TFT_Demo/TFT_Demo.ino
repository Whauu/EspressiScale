#include<TFT_GC9D01N.h>
#include "img.h"

TFT_GC9D01N_Class TFT_099;
char *str = "ABCabc123";

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    Serial.println("setup ");
    TFT_099.begin();
    TFT_099.backlight(50);

    TFT_099.DispColor(0, 0, TFT_WIDTH, TFT_HEIGHT, RED);
    delay(500);
    TFT_099.DispColor(0, 0, TFT_WIDTH, TFT_HEIGHT, BLUE);
    delay(500);
    TFT_099.DispColor(0, 0, TFT_WIDTH, TFT_HEIGHT, GREEN);
    delay(500);
    TFT_099.DispColor(0, 0, TFT_WIDTH, TFT_HEIGHT, BLACK);
    delay(500);
    TFT_099.DispStr(str, 0, 0, WHITE, BLACK);
    delay(2000);
    TFT_099.DrawImage(0, 0, 40, 40, closeX);
    TFT_099.DrawImage(0, 40, 40, 40, closeX);
    TFT_099.DrawImage(0, 40 * 2,  40, 40, closeX);
    TFT_099.DrawImage(0, 40 * 3, 40, 40, closeX);

    delay(2000);
}
int i = 0;
void loop()
{
    for (int j = 0; j < 4; j++) {
        TFT_099.DrawImage(0, (160 - (i + j * 55)), 30, 30, image_data_30);
    }
    i++;
    if (i == 55) {
        i = 0;
    }
}