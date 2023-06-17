
// libusbd480 1.1 - http://mylcd.sourceforge.net/

// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-20010  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.


#ifndef _LIBUSBD480_H_
#define _LIBUSBD480_H_


#include <usb.h>	// libusb0


#define USBD480_VID						0x16C0
#define USBD480_PID						0x08A6

#define USBD480_SET_ADDRESS				0xC0 // set SDRAM address
#define USBD480_WRITE_DATA				0xC1 // write one 16-bit word
#define USBD480_SET_FRAME_START_ADDRESS	0xC4 // set frame start address
#define USBD480_SET_STREAM_DECODER		0xC6
#define USBD480_TOUCH_READ				0xE1
#define USBD480_TOUCH_MODE				0xE2
#define USBD480_GET_DEVICE_DETAILS		0x80
#define USBD480_SET_CONFIG_VALUE		0x82
#define USBD480_GET_CONFIG_VALUE		0x83
#define USBD480_SAVE_CONFIG				0x84 // write config to flash
#define USBD480_GET_SAVED_CONFIG_VALUE  0x86


#define CFG_TOUCH_MODE					2
#define CFG_TOUCH_DEBOUNCE_VALUE		3	// default 50 - 0 to 254
#define CFG_TOUCH_SKIP_SAMPLES			4	// default 40
#define CFG_TOUCH_PRESSURE_LIMIT_LO		5	// default 30
#define CFG_TOUCH_PRESSURE_LIMIT_HI		6	// default 120
#define CFG_BACKLIGHT_BRIGHTNESS		20	// 0 - 255, default 255
#define CFG_USB_ENUMERATION_MODE		22	// 0 - 1, default 0. 0:HID device, 1:vender specific device

#define DEVICE_NAMELENGTH				63
#define DEVICE_SERIALLENGTH				10
#define DEBOUNCETIME					120		/* millisecond period*/
#define DRAGDEBOUNCETIME				50


#define DEBOUNCE(x) ((x)->pos.dt >= DEBOUNCETIME)
#define PIXELDELTAX(di, x) ((int)((x)*(4096.0/(float)(di)->Width)))
#define PIXELDELTAY(di, y) ((int)((y)*(4096.0/(float)(di)->Height)))
#define POINTDELTA 32
#define TBINPUTLENGTH 14


typedef struct {
	unsigned short x;
	unsigned short y;
}__attribute__((packed))TUSBD480TOUCHCOORD4;

typedef struct {
	unsigned short x;
	unsigned short y;
	unsigned short z1;
	unsigned short z2;
	unsigned char pen;
	unsigned char pressure;
	char padto16[6];
}__attribute__((packed))TUSBD480TOUCHCOORD16;

#ifndef TTOUCHCOORD30
typedef struct {
	int x;
	int y;
	int time;
	int dt;
	int z1;
	int z2;
	unsigned int count;	// this is n'th report
	unsigned char pen;
	unsigned char pressure;
}TTOUCHCOORD;
#endif

typedef struct {
	int mark;
	TTOUCHCOORD loc;
}TIN;

typedef struct {
	int left;
	int right;
	int top;
	int bottom;
	float hrange;
	float vrange;
	float hfactor;
	float vfactor;
}TCALIBRATION;

typedef struct {
	TTOUCHCOORD pos;		// current position
	TTOUCHCOORD last;		// previous position
	TIN tin[TBINPUTLENGTH+8];			
	TCALIBRATION cal;
	unsigned int count;		// number of filtered locations processed and accepted
	unsigned int dragState;
	int	dragStartCt;
}TTOUCH;

typedef struct{
	uint32_t Width;
    uint32_t Height;
    uint32_t PixelFormat;
	uint32_t DeviceVersion;
	char Name[DEVICE_NAMELENGTH+1];   
    char Serial[DEVICE_SERIALLENGTH+1];

	struct usb_device *usbdev; 
	usb_dev_handle *usb_handle;
	TTOUCH touch;
	TUSBD480TOUCHCOORD16 upos;
	int currentPage;
	char *tmpBuffer;			// used with embedded header frame updates.
}TUSBD480;


enum _PIXEL_FORMAT
{
	BPP_1BPP = 0,
	BPP_2BPP,
	BPP_4BPP,
	BPP_8BPP,
	BPP_16BPP_RGB565,
	BPP_16BPP_BGR565,
	BPP_32BPP
};


int libusbd480_OpenDisplay (TUSBD480 *di, uint32_t index);
int libusbd480_CloseDisplay (TUSBD480 *di);
int libusbd480_DrawScreen (TUSBD480 *di, uint8_t *fb, size_t size);
int libusbd480_DrawScreenIL (TUSBD480 *di, uint8_t *fb, size_t size);
int libusbd480_SetBrightness (TUSBD480 *di, uint8_t level);
int libusbd480_GetDeviceDetails (TUSBD480 *di, char *name, uint32_t *width, uint32_t *height, uint32_t *version, char *serial);
int libusbd480_WriteData (TUSBD480 *di, void *data, size_t size);
int libusbd480_SetConfigValue (TUSBD480 *di, int cfg, int value);
int libusbd480_GetConfigValue (TUSBD480 *di, int cfg, int *value);
int libusbd480_SaveConfig (TUSBD480 *di);
int libusbd480_GetSavedConfigValue (TUSBD480 *di, int cfg, int value);

// Equivalent to USBD480_SET_FRAME_START_ADDRESS
int libusbd480_SetFrameAddress (TUSBD480 *di, unsigned int addr);

// Equivalent to USBD480_SET_ADDRESS
int libusbd480_SetWriteAddress (TUSBD480 *di, unsigned int addr);

// Get and wait for a single touch report from controller
// If USBD480 is installed as a composite device then use hid_GetTouchPosition(). Refer to hid.c/h
// Both libusbd480_GetTouchPosition and hid_GetTouchPosition() block.
int libusbd480_GetTouchPosition (TUSBD480 *di, TTOUCHCOORD *pos);


// configure the stream decoder with firmware 0x700 or later
int libusbd480_EnableStreamDecoder (TUSBD480 *di);
int libusbd480_DisableStreamDecoder (TUSBD480 *di);
int libusbd480_SetLineLength (TUSBD480 *di, int value);
int libusbd480_SetBaseAddress (TUSBD480 *di, unsigned int address);



#endif 


