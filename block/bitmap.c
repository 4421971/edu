//////////////////////////////////////////////////////////////////////////
// bitmap.cpp
//////////////////////////////////////////////////////////////////////////

#include "bitmap.h"

//////////////////////////////////////////////////////////////////////////

#pragma pack(1)
typedef struct s_targa_file_header
{	
	BYTE IdentSize;
	BYTE ColorMapType;
	BYTE ImageType;
	WORD ColorMapStart;
	WORD ColorMapLength;
	BYTE ColorMapBits;
	WORD XStart;
	WORD YStart;
	WORD Width;
	WORD Height;
	BYTE Bits;
	BYTE Descriptor;
}targa_file_header_t;

typedef struct s_bitmap_file_header
{
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;    
}bitmap_file_header_t;

typedef struct s_bitmap_info_header
{
	DWORD      biSize;
	LONG       biWidth;
	LONG       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	LONG       biXPelsPerMeter;
	LONG       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} bitmap_info_header_t;

#pragma pack()

bitmap_t* load_bmp(const char* name)
{
	bitmap_file_header_t bf;
	bitmap_info_header_t bi;
	FILE* input = NULL;
	bitmap_t* bmp = NULL;
	int i, j;

	input = fopen(name, "rb");
	if(input == NULL) return NULL;
	fread(&bf, sizeof(bitmap_file_header_t), 1, input);
	fread(&bi, sizeof(bitmap_info_header_t), 1, input);
	if(bi.biCompression != 0)return NULL;

	bmp = (bitmap_t*)malloc(sizeof(bitmap_t));
	bmp->bits = bi.biBitCount;
	bmp->width = bi.biWidth;
	bmp->height = bi.biHeight;
	bmp->pixels =  (BYTE*)malloc(bmp->width * bmp->height * 4 * sizeof(BYTE)); //存放像素的字节

	switch (bmp->bits)
	{
	case 16:
		{
			for (i = 0; i < bmp->height; i++)
			{
				for (j = 0; j < bmp->width; j++)
				{
					WORD color;
					BYTE r, g, b, a;
					fread(&color, 1, sizeof(WORD), input);
					r = (( color >> 10 ) & 0x1f) << 3;
					g = (( color >> 5 ) & 0x1f) << 3;
					b = (color & 0x1f) << 3;
					a = 0xFF;	
					// 每个像素四个字节
					bmp->pixels[j * 4 + 0 + (bmp->height - i - 1) * bmp->width * 4] = b;
					bmp->pixels[j * 4 + 1 + (bmp->height - i - 1) * bmp->width * 4] = g;
					bmp->pixels[j * 4 + 2 + (bmp->height - i - 1) * bmp->width * 4] = r;
					bmp->pixels[j * 4 + 3 + (bmp->height - i - 1) * bmp->width * 4] = a;
				}
			}
		}
		break;
	case 24:
		{
			for (i = 0; i < bmp->height; i++)
			{
				int line_byte = (bmp->width*bmp->bits/8+3)/4*4;
				BYTE*line_buf = (BYTE*)malloc(line_byte*sizeof(BYTE));
				fread(line_buf, line_byte, sizeof(BYTE), input);
				for (j = 0; j < bmp->width; j++)
				{
					BYTE r, g, b, a;
					b = line_buf[j * 3 + 0];
					g = line_buf[j * 3 + 1];
					r = line_buf[j * 3 + 2];
					a = 0xff;
					bmp->pixels[j * 4 + 0 + ( bmp->height - i - 1 ) * bmp->width * 4] = b;
					bmp->pixels[j * 4 + 1 + ( bmp->height - i - 1 ) * bmp->width * 4] = g;
					bmp->pixels[j * 4 + 2 + ( bmp->height - i - 1 ) * bmp->width * 4] = r;
					bmp->pixels[j * 4 + 3 + ( bmp->height - i - 1 ) * bmp->width * 4] = a;
 				}
				free(line_buf);
			}
		}
		break;
	case 32:
		{
			for (i = 0; i < bmp->height; i ++)
			{
				for (j = 0; j < bmp->width; j++)
				{
					BYTE r, g, b, a;
					fread(&b, 1, sizeof(BYTE), input);
					fread(&g, 1, sizeof(BYTE), input);
					fread(&r, 1, sizeof(BYTE), input);
					fread(&a, 1, sizeof(BYTE), input);
					bmp->pixels[j * 4 + 0 + (bmp->height - i - 1) * bmp->width * 4] = b;
					bmp->pixels[j * 4 + 1 + (bmp->height - i - 1) * bmp->width * 4] = g;
					bmp->pixels[j * 4 + 2 + (bmp->height - i - 1) * bmp->width * 4] = r;
					bmp->pixels[j * 4 + 3 + (bmp->height - i - 1) * bmp->width * 4] = a;
				}
			}
		}
		break;
	default:
		break;
	}
	
	fclose(input);
	return bmp;
}

//////////////////////////////////////////////////////////////////////////