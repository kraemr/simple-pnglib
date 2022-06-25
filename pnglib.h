/*
MIT License

Copyright (c) 2022 Robin Krämer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/



#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <sys/stat.h>

//******** Variable Definitions ********//{

extern const uint8_t png_identifier[];
extern const uint8_t IDAT_ID[];
extern const uint8_t IDAT_END_ID[];
extern const uint8_t PLTE_identifier[];
extern const uint8_t IHDR_identifier[];
extern const uint8_t Trns_id[];
extern const uint8_t standard_ihdr_length[];
extern const uint8_t zeroes[];
extern struct pixel** pixel_rows;
typedef struct pixel** pixel_pp;
typedef struct pixel* pixel_p;


struct pixel
{
    uint8_t r; //red
    uint8_t g; // green 
    uint8_t b; // blue
    uint8_t A; // Alpha
};

struct TRNS_CHUNK{
    unsigned int length;
    uint8_t* Chunk_Data;
};

struct PLTE_CHUNK{
    unsigned int length; // 4 bytes 
    unsigned int Chunk_type; // 4 bytes
    uint8_t* Chunk_Data;//Data bytes according to length r would be index 0 green 1 blue 2 and so on
    unsigned int CRC; // for byte crc we will ignore this for now
};

struct IHDR_CHUNK{
    unsigned int width;
    unsigned int height;
    uint8_t bit_depth;
    uint8_t colour_type;
    uint8_t compression_method;
    uint8_t Filter_method;
    uint8_t Interface_Method;
};
//******** Variable Definitions ********//}



//######## Functions #########//{
int  PNG_Init(char* FileName,int Option);
void PNG_Exit();//Deallocates ALL internal buffers use this if you want to read another file
void PNG_Get_Dim(int* w, int* h);
void PNG_Get_Pixelpp(pixel_pp* pp); // gets 2d pixel struct array for really easy access to all values
void PNG_Get_Pixelvals(uint8_t** plist); // saves the IDAT buffer (the raw rgba vals) into a 1d uint8_t* (unsigned char*) buffer
void make2d_Idat(uint8_t*** out,uint8_t** in,int w, int h);
void PNG_Free_2dpixel(int height , pixel_pp* pp);
void mipmap(uint8_t** buf,uint8_t** scaled_out,int h, int w,int scale);
void Png_Encode(uint8_t *IDAT_input, char *PngName, int width, int height, int bytepp);
//######## Functions #########//}



