
#include "globals.h"
#include <stdio.h>

extern unsigned char * g_spng_IDAT_BUFFER_unfiltered;
//reads the give image and writes dimensions, 
//color and more to spnginf which is passed as a reference
int SPNG_read(char * filename,struct SPNG_INFO* spnginf);
void SPNG_get_pixels(unsigned char** pixelbuffer);//gets the pixel values without converting
void SPNG_get_pixels_greyscale(struct SPNG_INFO* spnginf,unsigned char** pixelbuffer,unsigned char withAlpha);
// spnginf holds the dimensions,clrtype and other things
int SPNG_write(FILE * fp,struct SPNG_INFO* spnginf,unsigned char* in_pix_buf);
int SPNG_write_indexed(FILE * fp,struct SPNG_INFO* spnginf,unsigned char* in_pix_buf);

void SPNG_exit();// deallocates internal Buffer call this when you are finished to avoid memleaks
void SPNG_get_pixels_srgb(struct SPNG_INFO* spnginf,unsigned char** pixelbuffer,unsigned char withAlpha);
int SPNG_get_spnginfo(struct SPNG_INFO* spnginf);
int SPNG_get_spnginfo_from_file(char * filename,struct SPNG_INFO* spnginf);
void SPNG_write_metadata(FILE * fp,struct SPNG_INFO* spnginf);
int SPNG_write_authorinfo(FILE * fp,struct SPNG_AUTHORINFO spngauthinf);
void SPNG_get_Authorinfo(struct SPNG_AUTHORINFO* spngauthinf);
void SPNG_free_author_info(struct SPNG_AUTHORINFO* info);
void SPNG_reset_author_info_lengths(struct SPNG_AUTHORINFO* info);