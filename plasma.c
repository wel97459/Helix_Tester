

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

		int dsize = (writelength + 1)*2;
		memcpy(&data[10], fb, dsize);
		dsize += 10;

		if ((dsize == (size_t)libusbd480_WriteData(di, data, dsize)))
			return 1;
		else
			printf("usbd480_DrawScreen(): usb_bulk_write() error %i\n", dsize);
	}
	return 0;
}

void hueToRGB(int hue, unsigned char *red, unsigned char *green, unsigned char *blue) {
    float h = (float)hue / 256; // Scale hue to the range [0, 1]
    float r, g, b;
    int i;
    float f, p, q, t;

    if (h >= 1.0)
        h = 0.0;

    h *= 6; // Scale hue to the range [0, 6]
    i = (int)h;
    f = h - i;
    p = 0;
    q = 1 - f;
    t = f;

    switch (i) {
        case 0:
            r = 1;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = 1;
            b = p;
            break;
        case 2:
            r = p;
            g = 1;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = 1;
            break;
        case 4:
            r = t;
            g = p;
            b = 1;
            break;
        case 5:
        default:
            r = 1;
            g = p;
            b = q;
            break;
    }

    *red = (unsigned char)(r * 255);
    *green = (unsigned char)(g * 255);
    *blue = (unsigned char)(b * 255);
}

void render_plasma(uint8_t *buffer, float time, int Width, int Height) {
    int x, y;
    unsigned char r,g,b;
    for (y = 0; y < Height; y++) {
        for (x = 0; x < Width; x++) {
            float value = sinf(x * 0.1f + time) + sinf(y * 0.15f + time) + sinf((x + y) * 0.1f + time);
            int intensity = (int)((value + 3.0f) * 56.0f);

            hueToRGB(intensity%360, &r,&g,&b);
            buffer[(y * Width*3) + (x*3)] = r;
            buffer[(y * Width*3) + (x*3)+1] = g;
            buffer[(y * Width*3) + (x*3)+2] = b;
        }
    }
}

int main (int argc, char **argv)
{
	TUSBD480 di;

	if (libusbd480_OpenDisplay(&di, 0)){		// open first device
        unsigned char *data = malloc(di.Width * di.Height * 3);

		printf("Name:%s\nWidth:%i\nHeight:%i\nVersion:0x%X\nSerial:%s\n", di.Name, di.Width, di.Height, di.DeviceVersion, di.Serial);

        uint16_t *data565 = (uint16_t *) malloc((di.Width * di.Height)*sizeof(uint16_t));
        float i = 0.8;
        while(1){
            render_plasma(data, i, di.Width, di.Height);
            rgbToRgb565Buffer(data, data565, di.Width * di.Height);
            if(DrawScreenX700(&di, (uint8_t *)data565)==0) goto quit;
            i=i+0.15;
            libusbd480_SetFrameAddress(&di, 0);
			libusbd480_SetWriteAddress(&di, 0);
        }
	    stbi_image_free(data);
	}
quit:
	libusbd480_CloseDisplay(&di);			// shutdown
	return 1;
}
