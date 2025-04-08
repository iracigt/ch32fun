// Small example showing how to use the USB HS interface of the CH32V30x
// A composite HID device + A bulk in and out.

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>
#include "hsusb_v30x.h"
#include "hsusb_v30x.c" // Normally would be in ADDITIONAL_C_FILES, but the PIO build system doesn't currently understand that.

uint8_t scratchpad[1024] = {0};

void HandleGotEPComplete( struct _USBState * ctx, int ep ) { }

int main()
{
	SystemInit();

	funGpioInitAll();

	funPinMode( PA3, GPIO_CFGLR_OUT_10Mhz_PP );

	HSUSBSetup();

	funDigitalWrite( PA3, FUN_LOW );

	while(1)
	{
		if( !( HSUSBCTX.USBHS_Endp_Busy[EP_DATA] & 1 ) )
		{
			USBHS_SendEndpoint(EP_DATA, 512, scratchpad );
			for (int i = 0; i < 512; i++) {
				scratchpad[i]++;
			}
		}
	}
}

