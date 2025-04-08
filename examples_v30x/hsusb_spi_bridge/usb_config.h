#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "funconfig.h"
#include "ch32v003fun.h"

#define HUSB_CONFIG_EPS       2 // Include EP0 in this count
#define HUSB_SUPPORTS_SLEEP   0
#define HUSB_HID_INTERFACES   0
#define HUSB_HID_USER_REPORTS 0
#define HUSB_IO_PROFILE       1

#define EP_DATA               1

#define HUSB_BULK_USER_REPORTS 1

#include "usb_defines.h"

//Taken from http://www.usbmadesimple.co.uk/ums_ms_desc_dev.htm
static const uint8_t device_descriptor[] = {
	18, //Length
	1,  //Type (Device)
	0x00, 0x02, //Spec
	0xEF, //Device Class
	0x02, //Device Subclass
	0x01, //Device Protocol  (000 = use config descriptor)
	64, //Max packet size for EP0
	0xFE, 0xCA, //ID Vendor
	0x00, 0x40, //ID Product
	0x01, 0x00, //ID Rev
	1, //Manufacturer string
	2, //Product string
	3, //Serial string
	1, //Max number of configurations
};


/* Configuration Descriptor Set */
static const uint8_t config_descriptor[ ] =
{
    /* Configuration Descriptor */
    0x09,                                                   // bLength
    0x02,                                                   // bDescriptorType
    0x19, 0x00,                                             // wTotalLength
    0x01,                                                   // bNumInterfaces
    0x01,                                                   // bConfigurationValue
    0x00,                                                   // iConfiguration
    0xA0,                                                   // bmAttributes: Bus Powered; Remote Wakeup
    0x32,                                                   // MaxPower: 100mA

    /* Interface Descriptor (data) */
    0x09,                                                   // bLength
    0x04,                                                   // bDescriptorType
    0x01,                                                   // bInterfaceNumber
    0x00,                                                   // bAlternateSetting
    0x01,                                                   // bNumEndpoints
    0xff,                                                   // bInterfaceClass
    0x00,                                                   // bInterfaceSubClass
    0x01,                                                   // bInterfaceProtocol
    0x04,                                                   // iInterface

    /* Endpoint Descriptor (data) */
    0x07,                                                   // bLength
    0x05,                                                   // bDescriptorType
    0x80 | EP_DATA,                                         // bEndpointAddress: IN Endpoint 1
    0x02,                                                   // bmAttributes
    0x00, 0x02,                                             // wMaxPacketSize
    0x01,                                                   // bInterval: 1mS
};



#define STR_MANUFACTURER u"Aperture Science Laboratories"
#define STR_PRODUCT      u"High-Speed Tunneling Device"
#define STR_SERIAL       u"CH32V307"
#define STR_VENDORITF	 u"SPI Data Interface"

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
const static struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
	4,
	3,
	{0x0409}
};
const static struct usb_string_descriptor_struct string1 __attribute__((section(".rodata")))  = {
	sizeof(STR_MANUFACTURER),
	3,
	STR_MANUFACTURER
};
const static struct usb_string_descriptor_struct string2 __attribute__((section(".rodata")))  = {
	sizeof(STR_PRODUCT),
	3,
	STR_PRODUCT
};
const static struct usb_string_descriptor_struct string3 __attribute__((section(".rodata")))  = {
	sizeof(STR_SERIAL),
	3,
	STR_SERIAL
};
const static struct usb_string_descriptor_struct string4 __attribute__((section(".rodata")))  = {
	sizeof(STR_VENDORITF),
	3,
	STR_VENDORITF
};

// This table defines which descriptor data is sent for each specific
// request from the host (in wValue and wIndex).
const static struct descriptor_list_struct {
	uint32_t	lIndexValue;
	const uint8_t	*addr;
	uint8_t		length;
} descriptor_list[] = {
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},

	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, sizeof(STR_MANUFACTURER)},
	{0x04090302, (const uint8_t *)&string2, sizeof(STR_PRODUCT)},	
	{0x04090303, (const uint8_t *)&string3, sizeof(STR_SERIAL)},
	{0x04090304, (const uint8_t *)&string4, sizeof(STR_VENDORITF)}
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)) )


#endif

