#include "ch32fun.h"
#include <stdio.h>

#define PIN_LED PA8
int main()
{
    SystemInit();

    funGpioInitAll(); // no-op on ch5xx

    funPinMode(PIN_LED, GPIO_CFGLR_OUT_10Mhz_PP);
    funDigitalWrite(PIN_LED, FUN_LOW);


    while (1)
    {
        funDigitalWrite(PIN_LED, FUN_HIGH);
        Delay_Ms(250);
        funDigitalWrite(PIN_LED, FUN_LOW);
        Delay_Ms(250);
    }
}
