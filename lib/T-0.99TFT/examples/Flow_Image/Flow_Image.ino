#include<TFT_GC9D01N.h>
#include "img.h"
TFT_GC9D01N_Class TFT_099;

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    Serial.println("setup ");
    TFT_099.begin();
    TFT_099.backlight(50);
    TFT_099.DispColor(0, 0, TFT_WIDTH, TFT_HEIGHT, BLACK);
    delay(100);

}
uint16_t i = 0;
void loop()
{
    for (int j = 0; j < 4; j++) {
        TFT_099.DrawImage(0, (160 - (i + j * 55)), 40, 40, cupcake);
    }
    i++;
    if (i == 55) {
        i = 0;
    }
}
