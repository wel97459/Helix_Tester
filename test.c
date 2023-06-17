
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
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <math.h>

#include "libusbd480.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#include "includes/stb/stb_image.h"

#include <stdint.h>
#include <stdlib.h>

void rgbToRgb565Buffer(const uint8_t* inputBuffer, uint16_t* outputBuffer, size_t bufferSize) {
    // Bit masks for RGB565 format
    uint16_t redMask = 0xF800;
    uint16_t greenMask = 0x07E0;
    uint16_t blueMask = 0x001F;

    for (size_t i = 0; i < bufferSize; i++) {
        uint8_t red = inputBuffer[i * 3];
        uint8_t green = inputBuffer[i * 3 + 1];
        uint8_t blue = inputBuffer[i * 3 + 2];

        // Shift and mask the RGB values
        uint16_t red565 = (red >> 3) << 11;
        uint16_t green565 = (green >> 2) << 5;
        uint16_t blue565 = blue >> 3;

        // Combine the shifted and masked values
        uint16_t rgb565 = red565 | green565 | blue565;

        outputBuffer[i] = rgb565;
    }
}

static int DrawScreenX700 (TUSBD480 *di, uint8_t *fb)
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
			printf("usbd480_DrawScreen(): usb_bulk_write() error %i\n", dsize);
	}
	return 0;
}

int main (int argc, char **argv)
{
	TUSBD480 di;

	if (libusbd480_OpenDisplay(&di, 0)){		// open first device
        int x,y,n;
        char fn[32];
        sprintf(fn, "%ux%u.jpg", di.Width, di.Height);
  
        unsigned char *data = stbi_load(fn, &x, &y, &n, 0);
        if(data == NULL){
            printf("Failed to load image: %s\n", fn);
            goto quit;
        }

        uint16_t *data565 = (uint16_t *) malloc((x*y)*sizeof(uint16_t));
        rgbToRgb565Buffer(data, data565, x*y);

		printf("Name:%s\nWidth:%i\nHeight:%i\nVersion:0x%X\nSerial:%s\n", di.Name, di.Width, di.Height, di.DeviceVersion, di.Serial);
        printf("image x:%i, y:%i, n:%i\n", x, y, n);

        DrawScreenX700(&di, (uint8_t *)data565);
	    stbi_image_free(data);
	}
quit:
	libusbd480_CloseDisplay(&di);			// shutdown
	return 1;
}
