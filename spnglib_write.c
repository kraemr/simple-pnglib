#include "include/spnglib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void spng_write_end(FILE* fp){
	const unsigned char IDAT_END_ID[]={
    0x49,0x45,0x4e ,0x44};
	const unsigned char zeroes[] ={0,0,0,0};
    fwrite(zeroes,1,4,fp);
    fwrite(IDAT_END_ID, 1, 4, fp);    
    unsigned char EOI[] = {0xae, 0x42, 0x60, 0x82};
    fwrite(EOI, 1, 4, fp);
}

unsigned long spng_deflate(unsigned char *t, int ScanLineLen, int height, int bytepp,int blocksize,int bits, FILE *fp)
{
    unsigned char out[blocksize];
    int e = 0;
    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.opaque = 0;
    defstream.next_in = (Bytef* )t;
    defstream.avail_in = (height*ScanLineLen);
    deflateInit(&defstream, Z_DEFLATED);
	unsigned char * png_crc_buf;
    unsigned long bytes_written=0;
    do
    {
        unsigned int have; // is also chnk length
        defstream.avail_out = blocksize;
        defstream.next_out = out;
        e = deflate(&defstream, Z_FINISH);
        have = blocksize - defstream.avail_out;
		unsigned int Chunk_length = have;
    	if(g_spng_is_little_endian)Chunk_length = __builtin_bswap32(Chunk_length);
    	fwrite(&Chunk_length,1,4,fp);
    	fwrite(g_spng_IDAT_ID, 1, 4, fp);
        bytes_written += fwrite(out, sizeof(char), have, fp);
        png_crc_buf = (unsigned char*)malloc(have+4);
        for(int i= 0; i < have; i++ ){
          png_crc_buf[i+4] = out[i]; 
        }
        memcpy(&png_crc_buf[0], g_spng_IDAT_ID, 4);
        unsigned long  crc_val = crc32(0L, Z_NULL, 0);
        crc_val = crc32(crc_val, (const unsigned char*)png_crc_buf, have+4);        
        if(g_spng_is_little_endian)crc_val = __builtin_bswap32(crc_val);
        fwrite(&crc_val,1,4,fp);
        free(png_crc_buf);
    } while (defstream.avail_out == 0);
    deflateEnd(&defstream);
    return bytes_written;
}

void SPNG_write_metadata(FILE * fp,struct SPNG_INFO* spnginf){
	const  unsigned char standard_ihdr_length[]={0x00,0x00,0x00,0x0d};
	const unsigned int zeroes [4]={0,0,0,0};
	unsigned int png_crc = 0;
	fwrite( g_spng_PNG_ID, 1, 8, fp);
	fwrite(standard_ihdr_length, 1,4,fp);
	fwrite(g_spng_IHDR_ID, 1,4,fp);
	unsigned int w = spnginf->width;
	unsigned int h = spnginf->height;
	w = (w >> 24) | ((w >> 8) & 0x0000ff00) | ((w<<8) & 0x00ff0000) | (w << 24);
	h = (h >> 24) | ((h >> 8) & 0x0000ff00) | ((h<<8) & 0x00ff0000) | (h << 24);
	fwrite(&w,1,4,fp);
	fwrite(&h,1,4,fp);
	fwrite(&spnginf->bitdepth, 1, 1, fp);
	fwrite(&spnginf->clr, 1, 1, fp);	
	fwrite(zeroes, 1, 3, fp);
	unsigned char ihdr_crc_buf[17];
	ihdr_crc_buf[0] = g_spng_IHDR_ID[0];
	ihdr_crc_buf[1] = g_spng_IHDR_ID[1];
	ihdr_crc_buf[2] = g_spng_IHDR_ID[2];
	ihdr_crc_buf[3] = g_spng_IHDR_ID[3];
    // gets the individual bytes out of the 4 bytes value
	ihdr_crc_buf[4] = (w & 0x000000ffUL);
	ihdr_crc_buf[5] = (w & 0x0000ff00UL) >> 8;
	ihdr_crc_buf[6] = (w & 0x00ff0000UL) >> 16;
	ihdr_crc_buf[7] = (w & 0xff000000UL) >> 24;
	ihdr_crc_buf[11] = (h & 0xff000000UL) >> 24;
	ihdr_crc_buf[10] = (h & 0x00ff0000UL) >> 16;
	ihdr_crc_buf[9] = (h & 0x0000ff00UL) >> 8;
	ihdr_crc_buf[8] = (h & 0x000000ffUL);
	ihdr_crc_buf[12] = spnginf->bitdepth;
	ihdr_crc_buf[13] = spnginf->clr;
	ihdr_crc_buf[14] = 0;
	ihdr_crc_buf[15] = 0;
	ihdr_crc_buf[16] = 0;
	png_crc = crc32(0,Z_NULL,0);
	png_crc = crc32(png_crc,ihdr_crc_buf,17);
	if(g_spng_is_little_endian)png_crc = __builtin_bswap32(png_crc);
	fwrite(&png_crc, 1,4, fp);
}



int spng_pixel_is_unique(struct SPNG_PIXEL plte[256],unsigned char * r, unsigned char * g, unsigned char * b,unsigned char * a,int len){
	for(int i = 0; i<len;i++)if((plte[i].r == (*r)) && (plte[i].g == (*g)) &&(plte[i].b == (*b)) &&(plte[i].a == (*a)) )return 0;  
	return 1;
}

void spng_plte_insert(struct SPNG_PIXEL plte[256],int plte_i,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
	plte[plte_i].r = r;
	plte[plte_i].g = g;
	plte[plte_i].b = b;
	plte[plte_i].a = a;
}

int spng_search_plte_pixel(struct SPNG_PIXEL plte[256],unsigned char r,unsigned char g,unsigned char b,unsigned char a){
	for(int i = 0; i < 256;i++){
		if(plte[i].r == r && plte[i].g == g && plte[i].b == b && plte[i].a == a)return i;
	}
	return -1; // doesnt exist
}





// needs a spnginf with proper width, height and clr corresponding to the passed buffer
// return SPNG_NULL if no filename is given no input buffer or no SPNG_INFO is passed
int SPNG_write(FILE * fp,struct SPNG_INFO* spnginf,unsigned char* in_pix_buf){
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif
	if(fp == NULL || spnginf == NULL || in_pix_buf == NULL){
		return SPNG_NULL; 
	}	
	unsigned int scanlinelength = spnginf->width * spnginf->bytespp+1;
	unsigned char * filtered_idat_buffer=(unsigned char *)malloc(spnginf->height * scanlinelength);
	int j=0;
		for(int i = 0;i < spnginf->height * (scanlinelength-1); i++){
			if(j % scanlinelength == 0 || j == 0){
				filtered_idat_buffer[j] = 0;
				j++;
			}
		filtered_idat_buffer[j] = in_pix_buf[i];
		j++;
		}
	spng_deflate(filtered_idat_buffer,scanlinelength,spnginf->height,spnginf->width,16384,8,fp);
	spng_write_end(fp);
	free(filtered_idat_buffer);
	fclose(fp);
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_end("write file");
	#endif
	return 0;
}

void spng_write_plte(FILE * fp,struct SPNG_PIXEL plte[256],unsigned int plte_size){
	unsigned char b[3];
	unsigned char PLTE_identifier[]={0x50,0x4c,0x54,0x45   };
	unsigned int png_crc= crc32(0, Z_NULL, 0);
	png_crc = crc32(png_crc, PLTE_identifier, 4);
	unsigned int temp_plte_size = plte_size*3;
	if(g_spng_is_little_endian)temp_plte_size = (temp_plte_size >> 24) | ((temp_plte_size >> 8) & 0x0000ff00) | ((temp_plte_size<<8) & 0x00ff0000) | (temp_plte_size << 24); //change endianness
	fwrite(&temp_plte_size,1,4,fp);
	fwrite(PLTE_identifier,1,4,fp);
	for(int i =0; i< plte_size;i++){
		b[0] = plte[i].r;
		b[1] = plte[i].g;
		b[2] = plte[i].b;
		fwrite(b, 1, 3, fp);
		png_crc = crc32(png_crc, b, 3);
	}
	if(g_spng_is_little_endian)png_crc = __builtin_bswap32(png_crc);
	fwrite(&png_crc, 1, 4, fp);
}

void spng_write_trns(FILE * fp,struct SPNG_PIXEL plte[256],unsigned int trns_size){
	unsigned int png_crc= crc32(0, Z_NULL, 0);
	png_crc = crc32(png_crc, g_spng_TRNS_ID, 4);
	unsigned int temp_trns_size = trns_size;
	if(g_spng_is_little_endian)temp_trns_size = (temp_trns_size >> 24) | ((temp_trns_size >> 8) & 0x0000ff00) | ((temp_trns_size<<8) & 0x00ff0000) | (temp_trns_size << 24); //change endianness
	fwrite(&temp_trns_size, 1, 4, fp);
	fwrite(g_spng_TRNS_ID,1, 4, fp);
	for(int i = 0; i < trns_size;i++){
		fputc(plte[i].a,fp);
		png_crc = crc32(png_crc, &plte[i].a, 1);
	}
	if(g_spng_is_little_endian)png_crc = __builtin_bswap32(png_crc);
	fwrite(&png_crc, 1, 4, fp);
}

// will try to write the given pixelbuffer as an indexed image
// if  it did that, then it will return 10 , which is defined as  
// and will instead write in the original clrtype of the given SPNG_INFO
// TODO add trns
int SPNG_write_indexed(FILE * fp,struct SPNG_INFO* spnginf,unsigned char* in_pix_buf){
	struct SPNG_PIXEL plte[256];
	int plte_i=0;
	unsigned char r,g,b,a;
	int has_trns = 0;
	if(spnginf->clr == 2){has_trns = 0;}
	else{has_trns = 1;}
	for(int i = spnginf->bytespp-1; i< spnginf->width*spnginf->height*spnginf->bytespp; i+=spnginf->bytespp){
		if(spnginf->clr == 2){
			r = in_pix_buf[i-2];
			g = in_pix_buf[i-1];
			b = in_pix_buf[i];
			a = 255;
		}else if(spnginf->clr == 6){
			r = in_pix_buf[i-3];
			g = in_pix_buf[i-2];
			b = in_pix_buf[i-1];
			a = in_pix_buf[i];
		}
		else if(spnginf->clr == 0){
			r = in_pix_buf[i+1];
			g = in_pix_buf[i+1];
			b = in_pix_buf[i+1];
			a = 255;
		}else{
			r = in_pix_buf[i-1];
			g = in_pix_buf[i-1];
			b = in_pix_buf[i-1];
			a = in_pix_buf[i];
		}
		if(plte_i > 255){
			return SPNG_write(fp, spnginf, in_pix_buf);
		}
		if(spng_pixel_is_unique(plte,&r,&g,&b,&a,plte_i )==1){
			spng_plte_insert(plte,plte_i, r, g, b, a);
			plte_i++;
		}
	}
	unsigned char tempclr=spnginf->clr;
	unsigned char tempbytepp=spnginf->bytespp;
	spnginf->clr = 3;
	spnginf->bytespp = 1;
	SPNG_write_metadata(fp, spnginf);
	spng_write_plte(fp,plte, plte_i);
	if(has_trns)spng_write_trns(fp, plte,  plte_i);
	spnginf->clr = tempclr;
	spnginf->bytespp = tempbytepp;
	int j = 0;
	unsigned char * new_indexed_pixel_buffer = (unsigned char *)malloc((spnginf->width+1) * spnginf->height);
	for(int i = 0; i < (spnginf->width)*spnginf->height*spnginf->bytespp-spnginf->bytespp; i+=spnginf->bytespp){
		if(j % (spnginf->width+1) == 0 || j == 0){
			new_indexed_pixel_buffer[j] = 0;
			j++;
		}
		switch (tempclr) {
			case 0: r= in_pix_buf[i];
			g=r;
			b=r;
			a=255;
			break;
			case 2:
			r= in_pix_buf[i];
			g = in_pix_buf[i+1];
			b = in_pix_buf[i+2];
			a = 255;
			break;
			case 4:
			r= in_pix_buf[i];
			g = r;
			b = r;
			a = in_pix_buf[i+1];
			break;
			case 6:
			r= in_pix_buf[i];
			g = in_pix_buf[i+1];
			b = in_pix_buf[i+2];
			a = in_pix_buf[i+3];
			break;
		}
		new_indexed_pixel_buffer[j] = spng_search_plte_pixel(plte,r,g,b,a);
		j++;
	}	
	spng_deflate(new_indexed_pixel_buffer,(spnginf->width+1), spnginf->height, 1,65535, 8, fp);
	spng_write_end(fp);
	fclose(fp);
	free(new_indexed_pixel_buffer);
	return 10;
}