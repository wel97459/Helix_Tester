
// libusbd480 1.1 - http://mylcd.sourceforge.net/

// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2010  Michael McElligott
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


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>


#include "libusbd480.h"


#define LIBUSB_VENDERID		USBD480_VID
#define LIBUSB_PRODUCTID	USBD480_PID
#define LIBUSB_FW			0x1		/* minimum required firmware revision*/


#ifdef __DEBUG__
#define umylog printf
#else
#define umylog(x, ...) 
#endif


uint32_t getTick() {
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime( CLOCK_REALTIME, &ts );
    theTick  = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}

int libusbd480_Init ()
{
	static int initOnce = 0;

	if (!initOnce){
		initOnce = 1;
		usb_init();
	}
    usb_find_busses();
    usb_find_devices();
	return 1;	
}

static void cleanQueue (TTOUCH *touch)
{
	int i;	
	for (i = 0; i <= TBINPUTLENGTH; i++){
		touch->tin[i].mark = 2;
		touch->tin[i].loc.x = 0;
		touch->tin[i].loc.y = 0;
		touch->tin[i].loc.pen = -1;
		touch->tin[i].loc.pressure = -1;
	}
	touch->dragState = 0;
	touch->dragStartCt = 0;
    touch->last.x = 0;
    touch->last.y = 0;
    touch->last.pen = -1;
	touch->last.time = getTick();
	touch->pos.time = touch->last.time;
}

void initTouch (TUSBD480 *di, TTOUCH *touch)
{
	cleanQueue(touch);
	touch->count = 0;
	touch->dragState = 0;
	touch->last.pen = -1;
	
	touch->cal.left = 210;		// your panel may differ 
	touch->cal.right = 210;
	touch->cal.top = 255;
	touch->cal.bottom = 300;
	touch->cal.hrange = (4095-touch->cal.left)-touch->cal.right;
	touch->cal.vrange = (4095-touch->cal.top)-touch->cal.bottom;
	touch->cal.hfactor = di->Width/touch->cal.hrange;
	touch->cal.vfactor = di->Height/touch->cal.vrange;
}

struct usb_device *find_usbd480 (int index)
{
    struct usb_bus *usb_bus;
    struct usb_device *dev;
    struct usb_bus *busses = usb_get_busses();
    
    for (usb_bus = busses; usb_bus; usb_bus = usb_bus->next) {
        for (dev = usb_bus->devices; dev; dev = dev->next) {
            if ((dev->descriptor.idVendor == LIBUSB_VENDERID) && (dev->descriptor.idProduct == LIBUSB_PRODUCTID)){
				//if (dev->config->interface->altsetting->bInterfaceNumber == 0){
            		if (index-- == 0){
						umylog("libusbd480: using device 0x%X:0x%X '%s'\n", dev->descriptor.idVendor, dev->descriptor.idProduct, dev->filename);
                		return dev;
                	}
                //}
			}
        }
    }
    return NULL;
}

int libusbd480_SaveConfig (TUSBD480 *di)
{
	int ret = 0;
	if (di){
		if (di->usb_handle){
			ret = usb_control_msg(di->usb_handle, 0x40, USBD480_SAVE_CONFIG, 0x8877, 0, NULL, 1, 500);
		}
	}
	return ret;
}

int libusbd480_GetSavedConfigValue (TUSBD480 *di, int cfg, int value)
{
	int ret = 0;
	if (di){
		if (di->usb_handle){
			char val = value&0xFF;
			ret = usb_control_msg(di->usb_handle, 0x40, USBD480_GET_SAVED_CONFIG_VALUE, cfg, 0, &val, 1, 500);
		}
	}
	return ret;
}

int libusbd480_SetConfigValue (TUSBD480 *di, int cfg, int value)
{
	int ret = 0;
	if (di){
		if (di->usb_handle){
			char val = value&0xFF;
			ret = usb_control_msg(di->usb_handle, 0x40, USBD480_SET_CONFIG_VALUE, cfg, 0, &val, 1, 500);
		}
	}
	return ret;
}

int libusbd480_GetConfigValue (TUSBD480 *di, int cfg, int *value)
{
	int ret = 0;
	if (di && value){
		if (di->usb_handle){
			char val = *value&0xFF;
			ret = usb_control_msg(di->usb_handle, 0xC0, USBD480_GET_CONFIG_VALUE, cfg, 0, &val, 1, 500);
			*value = val;
		}
	}
	return ret;
}

static int setConfigDefaults (TUSBD480 *di)
{
	int ret = libusbd480_SetConfigValue(di, CFG_TOUCH_MODE, 0);
	libusbd480_SetConfigValue(di, CFG_TOUCH_DEBOUNCE_VALUE, 20);
	libusbd480_SetConfigValue(di, CFG_TOUCH_SKIP_SAMPLES, 20);
	libusbd480_SetConfigValue(di, CFG_TOUCH_PRESSURE_LIMIT_LO, 50);
	libusbd480_SetConfigValue(di, CFG_TOUCH_PRESSURE_LIMIT_HI, 126);
	//libusbd480_SetConfigValue(di, CFG_USB_ENUMERATION_MODE, 0);
	//libusbd480_SetBrightness (di, 200);

#if 0
	// read back values
	int i;
	int value = 0;
	for (i = 0; i < 23; i++){
		libusbd480_GetConfigValue(di, i, &value);
		printf("config: %d value: %i\n", i, (unsigned char)value);
	}
#endif
	return ret;
}

int libusbd480_GetDeviceDetails (TUSBD480 *di, char *name, uint32_t *width, uint32_t *height, uint32_t *version, char *serial)
{
	char data[256];
	memset(data, 0, sizeof(data));
	const unsigned char rsize = 64;
	
	int result = usb_control_msg(di->usb_handle, 0xC0, USBD480_GET_DEVICE_DETAILS, 0, 0, data, rsize, 1000);
	if (result > 0){
		if (name) strncpy(name, data, DEVICE_NAMELENGTH);
		if (serial) strncpy(serial, &data[26], DEVICE_SERIALLENGTH);
		if (width) *width = (unsigned char)data[20] | ((unsigned char)data[21] << 8);
		if (height) *height = (unsigned char)data[22] | ((unsigned char)data[23] << 8);
		if (version) *version = (unsigned char)data[24] | ((unsigned char)data[25] << 8);
	}
	return result;
}

int libusbd480_OpenDisplay (TUSBD480 *di, uint32_t index)
{
	if (di == NULL)
		return -1;
		
	libusbd480_Init();
	memset(di, 0, sizeof(TUSBD480));
	
	di->usbdev = find_usbd480(index);
	if (di->usbdev){
		di->usb_handle = usb_open(di->usbdev); 
		if (di->usb_handle == NULL){
			umylog("libusbd480_OpenDisplay(): error: usb_open\n");
			return 0;
		}

		if (usb_set_configuration(di->usb_handle, 1) < 0){
			umylog("libusbd480_OpenDisplay(): error: setting config 1 failed\n");
			umylog("%s\n", usb_strerror());
			usb_close(di->usb_handle);
			return 0;
		}
		
		if (usb_claim_interface(di->usb_handle, 0) < 0){
			umylog("libusbd480_OpenDisplay(): error: claiming interface 0 failed\n");
			umylog("%s\n", usb_strerror());
			usb_close(di->usb_handle);
			return 0;
		}

		if (libusbd480_GetDeviceDetails(di, di->Name, &di->Width, &di->Height, &di->DeviceVersion, di->Serial)){
			umylog("libusbd480: name:'%s' width:%i height:%i version:0x%X serial:'%s'\n", di->Name, di->Width, di->Height, di->DeviceVersion, di->Serial);
			if (di->DeviceVersion < LIBUSB_FW){
				printf("libusbd480: firmware version 0x%X < 0x%X\n", di->DeviceVersion, LIBUSB_FW);
				printf("libusbd480 requires firmware 0x%X or later to operate effectively\n", LIBUSB_FW);
			}
			
			di->tmpBuffer = (char*)malloc((di->Width*di->Height*2)+1024); //framesize + headerspace + alignment
			if (di->tmpBuffer != NULL){
    			di->PixelFormat = BPP_16BPP_RGB565;
    			di->currentPage = 0;
				initTouch(di, &di->touch);
				setConfigDefaults(di);
				libusbd480_SetFrameAddress(di, 0);
				libusbd480_SetWriteAddress(di, 0);
				if (di->DeviceVersion == 0x700){
					libusbd480_EnableStreamDecoder(di);
					libusbd480_SetBaseAddress(di, 0);
					libusbd480_SetLineLength(di, di->Width);
				}
				return 1;
			}
		}else{
			umylog("libusbd480_GetDeviceDetails() failed\n");
		}
		libusbd480_CloseDisplay(di);
	}
	return 0;
}
                    
int libusbd480_CloseDisplay (TUSBD480 *di)
{
	if (di){
		if (di->usb_handle){
			usb_release_interface(di->usb_handle, 0);
			usb_close(di->usb_handle);
			di->usb_handle = NULL;
			if (di->tmpBuffer != NULL)
				free(di->tmpBuffer);
			di->tmpBuffer = NULL;
			return 1;
		}
	}
	return 0;
}

int libusbd480_DisableStreamDecoder (TUSBD480 *di)
{
	return usb_control_msg(di->usb_handle, 0x40, USBD480_SET_STREAM_DECODER, 0x00, 0, NULL, 0, 500);	
}

int libusbd480_EnableStreamDecoder (TUSBD480 *di)
{
	return usb_control_msg(di->usb_handle, 0x40, USBD480_SET_STREAM_DECODER, 0x06, 0, NULL, 0, 500);	
}

int libusbd480_SetLineLength (TUSBD480 *di, int length)
{
	char data[4];
	data[0] = 0x43; // wrap length
	data[1] = 0x5B;
	data[2] = (--length)&0xFF;
	data[3] = (length>>8)&0xFF;
	const int dsize = 4;
	int result;

	if ((result = libusbd480_WriteData(di, data, dsize)) != dsize)
		umylog("bulk_write error %i\n", result);
	return result;
}

int libusbd480_SetBaseAddress (TUSBD480 *di, unsigned int address)
{
	char data[6];
	data[0] = 0x42; // base address
	data[1] = 0x5B;
	data[2] = address&0xFF;
	data[3] = (address>>8)&0xFF;
	data[4] = (address>>16)&0xFF;
	data[5] = (address>>24)&0xFF;
	const int dsize = 6;
	int result;
	
	if ((result = libusbd480_WriteData(di, data, dsize)) != dsize)
		umylog("bulk_write error %i\n", result);
	return result;
}
				
int libusbd480_SetFrameAddress (TUSBD480 *di, unsigned int addr)
{
	return usb_control_msg(di->usb_handle, 0x40, USBD480_SET_FRAME_START_ADDRESS, addr, (addr>>16)&0xFFFF, NULL, 0, 1000);
}

int libusbd480_SetWriteAddress (TUSBD480 *di, unsigned int addr)
{
	return usb_control_msg(di->usb_handle, 0x40, USBD480_SET_ADDRESS, addr, (addr>>16)&0xFFFF, NULL, 0, 1000);
}

int libusbd480_WriteData (TUSBD480 *di, void *data, size_t size)
{
	return usb_bulk_write(di->usb_handle, 0x02, (char*)data, size, 1000);
}

static int libusbd480_DrawScreenX400 (TUSBD480 *di, uint8_t *fb, size_t size)
{
	if (di->usb_handle){
		di->currentPage ^= 0x01;
		unsigned int address;
		if (!di->currentPage)
			address = 0;
		else
			address = size>>1;
				
		libusbd480_SetWriteAddress(di, address);
		if ((size == (size_t)libusbd480_WriteData(di, fb, size))){
			libusbd480_SetFrameAddress(di, address);
			return 1;
		}else{
			umylog("usbd480_DrawScreen(): usb_bulk_write() error\n");
			return 0;
		}
	}
	return 0;
}

static int libusbd480_DrawScreenX700 (TUSBD480 *di, uint8_t *fb, size_t size)
{
	if (di->usb_handle){
		char *data = di->tmpBuffer;
		const int writeaddress = 0;
		const int writelength = (di->Width*di->Height) - 1; // (total pixels-1)
		
		data[0] = 0x41; // pixel write
		data[1] = 0x5B;
		data[2] = writeaddress&0xFF;
		data[3] = (writeaddress>>8)&0xFF;
		data[4] = (writeaddress>>16)&0xFF;
		data[5] = (writeaddress>>24)&0xFF;
		data[6] = writelength&0xFF;
		data[7] = (writelength>>8)&0xFF;
		data[8] = (writelength>>16)&0xFF;
		data[9] = (writelength>>24)&0xFF;

		int dsize = (writelength+1)*2;
		memcpy(&data[10], fb, dsize);
		dsize += 10;

		if ((dsize == (size_t)libusbd480_WriteData(di, data, dsize)))
			return 1;
		else
			umylog("usbd480_DrawScreen(): usb_bulk_write() error %i\n", dsize);
	}
	return 0;
}

int libusbd480_DrawScreen (TUSBD480 *di, uint8_t *fb, size_t size)
{
	if (di){
		if (di->DeviceVersion >= 0x700)
			return libusbd480_DrawScreenX700(di, fb, size);
		else
			return libusbd480_DrawScreenX400(di, fb, size);
	}
	return -1;
}

int libusbd480_DrawScreenIL (TUSBD480 *di, uint8_t *fb, size_t size)
{
	static int first = 0;

	if (di){
		if (di->usb_handle){
			unsigned int i;
			const int pitch = di->Width*2;
			char data[pitch*2];
			libusbd480_SetFrameAddress(di, 0);

			first ^= 0x01;
			for (i = first; i < di->Height; i += 2){
				memcpy(data, fb+(i*pitch), pitch+64);
				libusbd480_SetWriteAddress(di, i*di->Width);
				libusbd480_WriteData(di, data, pitch+64);
			}
			return 1;
		}
	}
	return 0;
}

int libusbd480_SetBrightness (TUSBD480 *di, uint8_t level)
{	
	return libusbd480_SetConfigValue(di, CFG_BACKLIGHT_BRIGHTNESS, level);
}

// convert from touch panel location to display coordinates
static int filter0 (TUSBD480 *di, TTOUCH *touch, TTOUCHCOORD *pos)
{
	if ((pos->y > touch->cal.top) && (pos->y < 4095-touch->cal.bottom) && (pos->x > touch->cal.left) && (pos->x < 4095-touch->cal.right)){
		pos->x = di->Width-(touch->cal.hfactor*(pos->x - touch->cal.right));
		pos->y = touch->cal.vfactor*(pos->y - touch->cal.top);
		touch->pos.x = pos->x;
		touch->pos.y = pos->y;
		return 1;
	}else{
		return 0;
	}
}


// detect and remove erroneous coordinates plus apply a little smoothing
static int filter1 (TUSBD480 *di, TTOUCH *touch)
{
	TTOUCHCOORD *pos = &touch->pos;
	memcpy(&touch->last, pos, sizeof(TTOUCHCOORD));

	// check current with next (pos)
	int dx = abs(touch->tin[TBINPUTLENGTH-1].loc.x - pos->x);
	int dy = abs(touch->tin[TBINPUTLENGTH-1].loc.y - pos->y);
	if (dx > PIXELDELTAX(di, POINTDELTA) || dy > PIXELDELTAY(di, POINTDELTA))
		touch->tin[TBINPUTLENGTH].mark = 1;
	else
		touch->tin[TBINPUTLENGTH].mark = 0;
	memcpy(&touch->tin[TBINPUTLENGTH].loc, pos, sizeof(TTOUCHCOORD));

	// check current with previous
	if (touch->tin[TBINPUTLENGTH-1].mark != 2 && touch->tin[TBINPUTLENGTH-2].mark != 2){
		dx = abs(touch->tin[TBINPUTLENGTH-1].loc.x - touch->tin[TBINPUTLENGTH-2].loc.x);
		dy = abs(touch->tin[TBINPUTLENGTH-1].loc.y - touch->tin[TBINPUTLENGTH-2].loc.y);
		if (dx > PIXELDELTAX(di, POINTDELTA) || dy > PIXELDELTAY(di, POINTDELTA))
			touch->tin[TBINPUTLENGTH-1].mark = 1;
		else
			touch->tin[TBINPUTLENGTH-1].mark = 0;
	}

	int i;
	for (i = 0; i < TBINPUTLENGTH; i++)
		memcpy(&touch->tin[i], &touch->tin[i+1], sizeof(TIN));

	if (touch->tin[TBINPUTLENGTH].mark != 0)
		return 0;

	int total = 0;
	pos->x = 0;
	pos->y = 0;
	
	for (i = 0; i < TBINPUTLENGTH; i++){
		if (touch->tin[i].mark){
			pos->x = 0;
			pos->y = 0;
			total = 0;
		}else if (/*!touch->tin[i].mark &&*/ touch->tin[i].loc.y){
			pos->x += touch->tin[i].loc.x;
			pos->y += touch->tin[i].loc.y;
			total++;
		}
	}
	if (total){
		const int t = getTick();
		pos->dt = t - pos->time;
		pos->time = t;
		pos->x /= (float)total;
		pos->y /= (float)total;
		pos->count = touch->count++;
		return total;
	}else{
		return 0;
	}
}

static int detectdrag (TUSBD480 *di, TTOUCH *touch)
{
	if (touch->pos.dt < DRAGDEBOUNCETIME){
		if (touch->dragState)
			return 1;
		
		const int dtx = abs(touch->pos.x - touch->last.x);
		const int dty = abs(touch->pos.y - touch->last.y);
		if ((dtx > PIXELDELTAX(di, 0) || dty > PIXELDELTAY(di, 0)) && dtx && dty){
			if (++touch->dragStartCt == 2){
				touch->dragState = 1;
				return 1;
			}
		}
	}
	return 0;
}

int process_inputReport (TUSBD480 *di, TUSBD480TOUCHCOORD16 *upos, TTOUCHCOORD *pos)
{
	pos->x = upos->x;
	pos->y = upos->y;
	pos->z1 = upos->z1;
	pos->z2 = upos->z2;
	pos->pen = upos->pen;
	pos->pressure = upos->pressure;
	memcpy(&di->touch.pos, pos, sizeof(TTOUCHCOORD));

	if (!pos->pressure && di->touch.last.pen == di->touch.pos.pen){
		cleanQueue(&di->touch);
		return -2;
	}else if (pos->pen == 1 && di->touch.last.pen == 0){	// pen up
		cleanQueue(&di->touch);
		return 3;
	}else if (pos->pen == 0 && di->touch.last.pen != 0){	// pen down
		cleanQueue(&di->touch);
	}

	if (filter1(di, &di->touch)){
		if (detectdrag(di, &di->touch)){
			if (filter0(di, &di->touch, &di->touch.pos)){
				pos->x = di->touch.pos.x;
				pos->y = di->touch.pos.y;
				pos->dt = di->touch.pos.dt;
				pos->time = di->touch.pos.time;
				pos->z1 = di->touch.pos.z1;
				pos->z2 = di->touch.pos.z2;
				pos->pen = di->touch.pos.pen;
				pos->pressure = di->touch.pos.pressure;
				pos->count = di->touch.pos.count;
				return 2;
			}
		}else if (DEBOUNCE(&di->touch)){
			if (filter0(di, &di->touch, &di->touch.pos)){
				pos->x = di->touch.pos.x;
				pos->y = di->touch.pos.y;
				pos->dt = di->touch.pos.dt;
				pos->time = di->touch.pos.time;
				pos->z1 = di->touch.pos.z1;
				pos->z2 = di->touch.pos.z2;
				pos->pen = di->touch.pos.pen;
				pos->pressure = di->touch.pos.pressure;
				pos->count = di->touch.pos.count;
				return 1;
			}
		}
	}
	return -2;
}

int libusbd480_GetTouchPosition (TUSBD480 *di, TTOUCHCOORD *pos)
{	
	if (di){
		if (di->usb_handle){
			int result = usb_interrupt_read(di->usb_handle, 0x81, (char*)&di->upos, 16, 1000);
			if (result != 16){
				if (result == -116){
					return -1;
				}else{
					printf("libusbd480_GetTouchPosition(): usb_interrupt_read() error: %d\n", result);
					return 0;
				}
			}else{
				return process_inputReport(di, &di->upos, pos);
			}
		}
	}
	return 0;
}

