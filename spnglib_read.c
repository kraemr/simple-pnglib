#include "include/spnglib.h"
#include "include/zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

unsigned char * g_spng_IDAT_BUFFER;
unsigned char * g_spng_IDAT_BUFFER_unfiltered;


extern void spng_change_endian(unsigned int * n);
extern void spng_read_author_title_desc(FILE * fp,int chnklen,int iscompressed,struct SPNG_AUTHORINFO* spng_auth);

// returns 1 if little endian and 0 if not 
unsigned char spng_cpu_get_is_little_endian(){
	int num = 1;
	return (*(char*)&num);
}


//calculates size of the resulting pixelbuffer
void spng_get_size(struct SPNG_INFO* spnginf){
	spnginf->size = (*spnginf).width * (*spnginf).height  * (*spnginf).bytespp;
}

// takes in spng_info and calculates bytespp
void spng_get_bytespp(struct SPNG_INFO* spnginf){
	switch ((*spnginf).clr) {
	case 6: (*spnginf).bytespp = 4 * spnginf->bitdepth / 8 ;break; // RGBA
	case 2: (*spnginf).bytespp = 3 * spnginf->bitdepth / 8;break; // RGB
	case 4: (*spnginf).bytespp = 2 * spnginf->bitdepth / 8;break; // GreyAlpha
	case 0: (*spnginf).bytespp = 1 * spnginf->bitdepth / 8;break; // Greyscale
	case 3: (*spnginf).bytespp = 1 * spnginf->bitdepth / 8;break; // indexed
	default:;
	}
}

unsigned int spng_chunk_length(FILE * fp){
	g_spng_is_little_endian = spng_cpu_get_is_little_endian();
	fseek(fp, -8, SEEK_CUR);
	unsigned int size = 0;
	fread(&size,1,4,fp);
	fseek(fp,4,SEEK_CUR);
	if(g_spng_is_little_endian)spng_change_endian(&size);
	return size;
}



void spng_read_until_IDAT(FILE * fp,struct SPNG_INFO* spnginf,unsigned char skip_anc){
	// starts after IHDR and reads all 
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif

	unsigned char b[4];
	unsigned int chunk_len;
	SPNG_free_author_info(&g_spng_author_info);
	SPNG_reset_author_info_lengths(&g_spng_author_info);
	while(!feof(fp)){
		fread(b,1,4,fp);
		chunk_len=spng_chunk_length( fp);
		if(memcmp(b, g_spng_IDAT_ID, 4)==0){
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				spng_bench_end("read until IDAT");
			#endif
			return;
		}
		if(memcmp(b, g_spng_tIME_ID, 4) == 0 ||memcmp(b,g_spng_iCCP_ID,4) == 0){
			fseek(fp,chunk_len,SEEK_CUR);
			fseek(fp,4,SEEK_CUR);
			continue;

		}

		if(
		memcmp(b,g_spng_iTXt_ID,4) == 0 ||
		memcmp(b,g_spng_tExT_ID,4) == 0) // can always occur in the file 
		{
			//fseek(fp,chunk_len,SEEK_CUR);
			spng_read_author_title_desc(fp,chunk_len,0,&g_spng_author_info);
			fseek(fp,4,SEEK_CUR);
			//fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}
		if(memcmp(b,g_spng_zTXt_ID,4) == 0 ){
			//fseek(fp,chunk_len+4,SEEK_CUR);
			spng_read_author_title_desc(fp,chunk_len,1,&g_spng_author_info);
			fseek(fp,4,SEEK_CUR);
			//fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}

		if(spnginf ->clr == 3 && memcmp(b,g_spng_PLTE_ID,4) == 0){
			fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}

		if(spnginf ->clr == 3 && memcmp(b,g_spng_TRNS_ID,4) == 0){
			fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}
		
		if(memcmp(b, g_spng_cHRM_ID, 4) == 0 ||
		memcmp(b, g_spng_gAMA_ID, 4) == 0 ||
		memcmp(b, g_spng_iCCP_ID, 4) == 0 ||
		memcmp(b, g_spng_sBIT_ID, 4) == 0  ||
		memcmp(b, g_spng_sRGB_ID, 4) == 0) 
		{
			fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}

		if(memcmp(b, g_spng_bkgd_ID, 4) == 0)
		{
			fseek(fp,4,SEEK_CUR);
			continue;	
		}

		if(	memcmp(b, g_spng_hIST_ID, 4) == 0 ||
			memcmp(b, g_spng_phys_ID, 4) == 0){
			fseek(fp,chunk_len+4,SEEK_CUR);
			continue;	
		}
	}
}

void spng_inflate_ng(unsigned int size,struct SPNG_INFO* spnginf){
	g_spng_IDAT_BUFFER_unfiltered = (unsigned char *)malloc((spnginf->bytespp * spnginf->width+1) * spnginf->height);
	z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = size;
    stream.next_in = g_spng_IDAT_BUFFER;
	stream.avail_out = (spnginf->bytespp * spnginf->width+1) * spnginf->height;
	stream.next_out = g_spng_IDAT_BUFFER_unfiltered;
	inflateInit(&stream);
	inflate(&stream,Z_NO_FLUSH);
	inflateEnd(&stream);
	free(g_spng_IDAT_BUFFER);
}

void spng_read_IDAT(FILE * fp,struct SPNG_INFO* spnginf,int start){
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif
	g_spng_IDAT_BUFFER = (unsigned char *)malloc(((spnginf->width * spnginf->bytespp)+1) * spnginf->height );
	unsigned char b[4]={0};
	fseek(fp,start,SEEK_SET);
	spng_read_until_IDAT(fp, &g_spng_spnginf, 1);
	fseek(fp,-4,SEEK_CUR);
	unsigned int total_len=0;
	while(!feof(fp)){
		fread(b,1,4,fp);
			if(memcmp(b,g_spng_IDAT_ID, 4) == 0) {
				int chunk_len = spng_chunk_length(fp);
				fread(g_spng_IDAT_BUFFER,1,chunk_len,fp);
				g_spng_IDAT_BUFFER+= chunk_len;
				total_len+= chunk_len;
				fseek(fp, 4, SEEK_CUR);
			}
	}
	g_spng_IDAT_BUFFER -= total_len;
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_end("read IDAT");
	#endif
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif
	spng_inflate_ng(total_len,spnginf);
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_end("spng inflate_ng");
	#endif
}


// clr type width height and misc values
// returns -1 if not a png
int SPNG_get_spnginfo_from_file(char * filename,struct SPNG_INFO* spnginf){
// read the idhr and return info
#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif
	if(filename==NULL) return SPNG_ERROR;
	FILE * fp = fopen(filename,"rb");
	if(fp==NULL){
		fclose(fp);
		return SPNG_FILE_NOT_FOUND;
	}
	unsigned char png_id[8]={0};
	fread(png_id,1,8,fp);
	if( memcmp(png_id,g_spng_PNG_ID,8) == 1)return SPNG_ERROR;
	g_spng_is_little_endian = spng_cpu_get_is_little_endian();
	unsigned int len = 0;
	fread(&len , 1, 4, fp); // chunk len
	fseek(fp,4,SEEK_CUR); // return to id
	fread( &(*spnginf).width,1,4,fp);// read width (Big Endian)
	if(g_spng_is_little_endian)spng_change_endian(&(*spnginf).width); // Convert to little endian
	fread( &(*spnginf).height,1,4,fp);// read height (Big Endian)
	if(g_spng_is_little_endian)spng_change_endian(&(*spnginf).height);// Convert to little endian
	fread( (&(*spnginf).bitdepth),1,1,fp);
	fread( (&(*spnginf).clr ),1,1,fp);
	spng_get_bytespp(spnginf);
	spng_get_size(spnginf);
	fseek(fp,7,SEEK_CUR);
	int fpos = ftell(fp);
	fclose(fp);
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_end("spnginfo read");
	#endif
	return fpos;
}


//writes to passed spnginf reference the internal buffers dimensions,bytespp,clrtype ...
//and if there is no internal buffer allocated it will return -3
//if succesful it will return 0
int SPNG_get_spnginfo(struct SPNG_INFO* spnginf){
	if(g_spng_is_allocated == 1){
		(*spnginf) = g_spng_spnginf;
		return SPNG_SUCCESS;
	}
	else return SPNG_NOT_ALLOCATED;
}


void spng_get_plte(char * fname,int fpos){
    unsigned char b[4];
	unsigned int len=0;//plte len
	unsigned char plte_buffer[1024];
	unsigned char trns_buffer[1024];
	g_spng_has_trns = 0; 
	FILE * fp=fopen(fname,"rb");
	fseek(fp, fpos, SEEK_SET);
    while(!feof(fp)){
        g_spng_bytes_rwritten = fread(b,1,1,fp);
        if(b[0] == g_spng_PLTE_ID[0]){
            fseek(fp,-1,SEEK_CUR); //go back one to read the whole thing
            g_spng_bytes_rwritten = fread(b,1,4,fp);
            if(memcmp(&b[0], g_spng_PLTE_ID, 4)==0){ // check if the 4 bytes match the identifier
				len = spng_chunk_length(fp);
				g_spng_plte_len = len/3;
                break;
            }
        }
    }
	g_spng_plte_pixels = (struct SPNG_PIXEL*)malloc(len/3 * sizeof(struct SPNG_PIXEL));
	g_spng_plte_is_allocated = 1;
	g_spng_bytes_rwritten = fread(plte_buffer,1,len,fp);
	int limit = 30000;
	  while(!feof(fp) && ftell(fp)<30000){
        g_spng_bytes_rwritten = fread(b,1,1,fp);
        if(b[0] == g_spng_TRNS_ID[0]){
            fseek(fp,-1,SEEK_CUR); //go back one to read the whole thing
            g_spng_bytes_rwritten = fread(b,1,4,fp);
            if(memcmp(&b[0], g_spng_TRNS_ID, 4)==0){ // check if the 4 bytes match the identifier
				g_spng_has_trns = 1;
				len = spng_chunk_length(fp);
                break;
            }
        }
    }
	g_spng_bytes_rwritten = fread(trns_buffer,1,len,fp);
	unsigned int trns_i = 0;
	unsigned int plte_i=0;
	unsigned int g_spng_end = g_spng_plte_len * 3;
	for(unsigned int i = 2; i < g_spng_end;i+=3){
		g_spng_plte_pixels[plte_i].r = plte_buffer[i-2];
		g_spng_plte_pixels[plte_i].g = plte_buffer[i-1];
		g_spng_plte_pixels[plte_i].b = plte_buffer[i];
		if(g_spng_has_trns == 1){
		g_spng_plte_pixels[plte_i].a = trns_buffer[trns_i];
		}
		else{
			g_spng_plte_pixels[plte_i].a = 255;
		}
		plte_i++;
		trns_i++;
	}
	fclose(fp);
}


// used to predict next pixel 
unsigned char spng_paeth_pred(unsigned char a,  unsigned char b, unsigned char c){
    int  p =a + b - c;
    int pa = abs(p - a);
    int  pb = abs(p - b);
    int pc = abs(p - c);
    unsigned char PR=0; // predictor
    if(pa <= pb && pa <= pc)PR = a;
    else if(pb <= pc)PR = b;
    else PR = c;
    return PR;
}



void spng_undo_paeth(struct SPNG_INFO* spnginf,unsigned int pos,unsigned int len_nofilter){
	//register unsigned char a,b,c;
	unsigned int last_scan_curr_i=pos-len_nofilter;// last scanlines current index
	unsigned int last_block_i=pos; // previous block of the current index in current line
	unsigned int last_scan_last_block_i=last_scan_curr_i; // the previous block of the current index in the previous line
	unsigned int i = pos;
	unsigned int j = i;
	unsigned int bytepp = spnginf->bytespp;

	for(; j<i + bytepp; j++){
				g_spng_IDAT_BUFFER_unfiltered[j]+=spng_paeth_pred(0, g_spng_IDAT_BUFFER_unfiltered[last_scan_curr_i], 0);
				last_scan_curr_i++;
	}
	i+=bytepp;

	while(i<pos+len_nofilter){
			for(j=i;j<i+bytepp;j++){
				g_spng_IDAT_BUFFER_unfiltered[j]+=spng_paeth_pred(g_spng_IDAT_BUFFER_unfiltered[last_block_i], g_spng_IDAT_BUFFER_unfiltered[last_scan_curr_i], g_spng_IDAT_BUFFER_unfiltered[last_scan_last_block_i]);
				last_block_i++;
				last_scan_last_block_i++;
				last_scan_curr_i++;
			}
		i+=bytepp;
	}

}


// no xmm
void spng_undo_sub(struct SPNG_INFO* spnginf,unsigned int len_nofilter,unsigned int pos){
	unsigned int k;
	for(unsigned int i=pos+spnginf->bytespp;i<pos+len_nofilter;i+=spnginf->bytespp){
	//if(i!=pos){//if it is not the first
		k = i;
		for(unsigned int j=i-spnginf->bytespp;j < i;j++,k++){
			g_spng_IDAT_BUFFER_unfiltered[k] += g_spng_IDAT_BUFFER_unfiltered[j];
		}
	//}
	}
}
void spng_undo_up(unsigned int len_nofilter,unsigned int pos){
	if(pos==0)return; // dont do anything as adding 0's wont make a difference anyway
	unsigned int j = 0;
	for(unsigned int i = pos,j = pos-len_nofilter;i<pos+len_nofilter;i++,j++){
		g_spng_IDAT_BUFFER_unfiltered[i] += g_spng_IDAT_BUFFER_unfiltered[j];
	}
}

void spng_undo_avg(struct SPNG_INFO* spnginf,unsigned int len_nofilter,unsigned int pos){
	unsigned int a,b;
	unsigned int lastblock_i=pos;
	unsigned int last_scan_current_i=pos-len_nofilter;
	unsigned int i = pos;
	unsigned int j=i;
	for(;i< pos+len_nofilter;i+=spnginf->bytespp){
		if(i!=pos){
			for(j=i;j<i+spnginf->bytespp ; ++j){ 
				a = g_spng_IDAT_BUFFER_unfiltered[lastblock_i];
				b = g_spng_IDAT_BUFFER_unfiltered[last_scan_current_i];
				last_scan_current_i++;
				lastblock_i++;
				g_spng_IDAT_BUFFER_unfiltered[j]+=((a+b)/2);
			}
		}else{
			for(j=i;j<i+spnginf->bytespp ; ++j){
				b = g_spng_IDAT_BUFFER_unfiltered[last_scan_current_i];
				last_scan_current_i++;
				g_spng_IDAT_BUFFER_unfiltered[j]+=(b/2);
		}
		}
		
	}
}
//TODO: all of the unfiltering inplace
void spng_undo_filters(struct SPNG_INFO* spnginf){
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_start();
	#endif
	unsigned int len = 1+(spnginf->width) * spnginf->bytespp;
	unsigned int len_nofilter = len-1;
	unsigned int i=0 , j = 0;
	unsigned char FILTER_VALUES[16384]={0};
	unsigned int buffer_size = len*spnginf->height;
	for(;i<buffer_size;i+=len,++j){
		FILTER_VALUES[j] = g_spng_IDAT_BUFFER_unfiltered[i];	
	} // after extracting the filter values remove them from g_spng_IDAT_BUFFER_unfiltered

	i=0;
	j=i;
	unsigned int k =i;

	for(;i<buffer_size; i+=len, j+=len_nofilter){
	memmove(&g_spng_IDAT_BUFFER_unfiltered[i-k],&g_spng_IDAT_BUFFER_unfiltered[i+1],len_nofilter); 
	++k;
	// move memory of an offset of 1(skipping the filter) into the memory section of the filter
	}

	j=0;
	i=0;
	for(;i<spnginf->height;++i,j+=len_nofilter){
		switch (FILTER_VALUES[i]) { //filter value is at this position
			case 0:
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				++NOFILTER_COUNT;
			#endif
			continue;break; //NOFILTER Do nothing
			case 1:
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				++SUB_COUNT;
			#endif
			spng_undo_sub(spnginf, len_nofilter, j);break; //SUB
			case 2:
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				++UP_COUNT;
			#endif
			spng_undo_up(len_nofilter, j);break; //UP
			case 3:
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				++AVG_COUNT;
			#endif
			spng_undo_avg(spnginf,len_nofilter, j);;break; //AVG
			case 4:
			#ifdef SPNGLIB_DEBUG_BENCHMARK
				++PAETH_COUNT;
			#endif
			spng_undo_paeth(spnginf, j, len_nofilter);break; //PAETH
		}
	}
	#ifdef SPNGLIB_DEBUG_BENCHMARK
		spng_bench_end("undo filters");
		printf("NO:%d\nSUB:%d\nUP:%d\nAVG:%d\nPAETH:%d\n",NOFILTER_COUNT,SUB_COUNT,UP_COUNT,AVG_COUNT,PAETH_COUNT);
	#endif
}

void SPNG_exit(){ 
	// if global Idat buffer is allocated(PNG was read with SPNG_read),then free it
	if(g_spng_is_allocated){ 
	free(g_spng_IDAT_BUFFER_unfiltered);
	g_spng_is_allocated=0; 
	}
	// if an indexed image was read then free the internal buffer
	if(g_spng_plte_is_allocated){
	free(g_spng_plte_pixels);
	g_spng_plte_is_allocated=0;
	}
	//SPNG_free_author_info(&g_spng_author_info);
	
}





//copies the internal pixelbuffer to the given pointer reference, which will be allocated by this function
void SPNG_get_pixels(unsigned char** pixelbuffer){
	(*pixelbuffer) = (unsigned char *)malloc(g_spng_spnginf.width * g_spng_spnginf.height * g_spng_spnginf.bytespp);
	memcpy((*pixelbuffer),g_spng_IDAT_BUFFER_unfiltered,g_spng_spnginf.width*g_spng_spnginf.height*g_spng_spnginf.bytespp);
}

// copies the internal buffer to the given pixelbuffer and allocates it by itself
// DO NOT ALLOCATE THE BUFFER BEFORE PASSING IT HERE
void SPNG_get_pixels_srgb(struct SPNG_INFO* spnginf,unsigned char** pixelbuffer,unsigned char withAlpha){
	int j = withAlpha ? 3 : 2;
	//internally rgb and requested rgb
	if(g_spng_spnginf.clr == 2 && withAlpha == 0){// img is in the same format as requested
		SPNG_get_pixels(pixelbuffer);
		return;
	}
	//internally rgba and requested rgba
	else if(g_spng_spnginf.clr == 6 && withAlpha == 1){ // img is in the same format as requested
		SPNG_get_pixels(pixelbuffer);
		return;
	}

	if(withAlpha){
		(*pixelbuffer)= (unsigned char *)malloc(g_spng_spnginf.width * g_spng_spnginf.height* 4);
		spnginf->bytespp = 4;
		spnginf->clr = 6;
	}else{
		(*pixelbuffer)= (unsigned char *)malloc(g_spng_spnginf.width * g_spng_spnginf.height* 3);
		spnginf->bytespp = 3;		
		spnginf->clr = 2;
	}

	if(g_spng_spnginf.clr == 3) { //indexed
		for(int i = 0; i< g_spng_spnginf.width*g_spng_spnginf.height;i++){
			if(withAlpha){
				(*pixelbuffer)[j-3] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].r;
				(*pixelbuffer)[j-2] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].g;
				(*pixelbuffer)[j-1] =   g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].b;
				(*pixelbuffer)[j] =   g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].a;
				j+=4;
			}else{
				(*pixelbuffer)[j-2] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].r;
				(*pixelbuffer)[j-1] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].g;
				(*pixelbuffer)[j] =   g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].b;
				j+=3;
			
			}
		}
		return;
	}
	if(g_spng_spnginf.clr == 0){//greyscale
		for(int i = 0; i < g_spng_spnginf.width*g_spng_spnginf.height;i++){
		if(withAlpha){
			(*pixelbuffer)[j-3] = g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j-1] =   g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j] = 255;
			j+=4;
		}else{
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j] =   g_spng_IDAT_BUFFER_unfiltered[i];
			j+=3;
		}
		}
	}
	if(g_spng_spnginf.clr == 4){//greyscale alpha
		for(int i = 1; i < g_spng_spnginf.width*g_spng_spnginf.height*2;i+=2){
		if(withAlpha){
			(*pixelbuffer)[j-3] = g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j-1] =   g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j] = g_spng_IDAT_BUFFER_unfiltered[i];
			j+=4;
		}else{//ignore alpha
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j] =   g_spng_IDAT_BUFFER_unfiltered[i-1];
			j+=3;
		}
		}
	}
	if(g_spng_spnginf.clr == 2 && withAlpha){
		for(int i = 2; i < g_spng_spnginf.width*g_spng_spnginf.height*3;i+=3){
			(*pixelbuffer)[j-3] = g_spng_IDAT_BUFFER_unfiltered[i-2];
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i-1];
			(*pixelbuffer)[j-1] =   g_spng_IDAT_BUFFER_unfiltered[i];
			(*pixelbuffer)[j] = 255;
			j+=4;
		}
	}
	if(g_spng_spnginf.clr == 6 && withAlpha==0){
		for(int i = 3; i < g_spng_spnginf.width*g_spng_spnginf.height*4;i+=4){
			(*pixelbuffer)[j-2] = g_spng_IDAT_BUFFER_unfiltered[i-3];
			(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i-2];
			(*pixelbuffer)[j] =   g_spng_IDAT_BUFFER_unfiltered[i-1];
			j+=3;
		}
	}
}

float spng_round_nearest(float f){
    int i = (int)f;
    float fcom = f - i; 
    if(fcom >= 0.5f){
        return f-fcom+1;
    }else{
        return f-fcom;
    }
}
unsigned char spng_calculate_greyscale(unsigned char r,unsigned char g,unsigned char b){
	    return spng_round_nearest((float)r * 0.299f) + spng_round_nearest((float)g *0.587f )+spng_round_nearest( (float)b * 0.114f);
}

void SPNG_get_pixels_greyscale(struct SPNG_INFO* spnginf,unsigned char** pixelbuffer,unsigned char withAlpha){
	
	if(g_spng_spnginf.clr == 0 && withAlpha == 0){
		SPNG_get_pixels(pixelbuffer);
		return;
	}
	else if(g_spng_spnginf.clr == 4 && withAlpha == 1){
		SPNG_get_pixels(pixelbuffer);
		return;
	}

	if(withAlpha){
		(*pixelbuffer)= (unsigned char*)malloc(g_spng_spnginf.width * g_spng_spnginf.height*2);
		spnginf->clr = 4;
		spnginf->bytespp = 2;
	}else{
		(*pixelbuffer)= (unsigned char*)malloc(g_spng_spnginf.width * g_spng_spnginf.height);
		spnginf->clr = 0;
		spnginf->bytespp = 1;
	}

	int j=	 withAlpha ? 1 : 0;
	if(g_spng_spnginf.clr == 3){
		for(int i = 0; i< g_spng_spnginf.width*g_spng_spnginf.height;i++){
			if(withAlpha){
				(*pixelbuffer)[j-1] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].r;
				(*pixelbuffer)[j-1] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].g;
				(*pixelbuffer)[j-1] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].b;
				(*pixelbuffer)[j] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].a;
				j+=2;
			}else{
				(*pixelbuffer)[j] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].r;
				(*pixelbuffer)[j] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].g;
				(*pixelbuffer)[j] = g_spng_plte_pixels[ g_spng_IDAT_BUFFER_unfiltered[i]].b;
				j++;
			}
		}
		return;
	}

	//convert from greyscale to greyscale alpha
	if(g_spng_spnginf.clr == 0 && withAlpha == 1){
		for(int i = 0; i< g_spng_spnginf.width*g_spng_spnginf.height;i++){
				(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i];
				(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i];
				(*pixelbuffer)[j-1] = g_spng_IDAT_BUFFER_unfiltered[i];
				(*pixelbuffer)[j] = 255;
				j+=2;
		}
		return;
	}
	//convert from rgb to greyscale or greyscale alpha
	if(g_spng_spnginf.clr == 2){
		for(int i = 2; i< g_spng_spnginf.width*g_spng_spnginf.height*3;i+=3){
			if(withAlpha){
				(*pixelbuffer)[j-1] = spng_calculate_greyscale(g_spng_IDAT_BUFFER_unfiltered[i-2],g_spng_IDAT_BUFFER_unfiltered[i-1],g_spng_IDAT_BUFFER_unfiltered[i]);
				(*pixelbuffer)[j] = 255;
				j+=2;
			}else{
				(*pixelbuffer)[j] = spng_calculate_greyscale(g_spng_IDAT_BUFFER_unfiltered[i-2],g_spng_IDAT_BUFFER_unfiltered[i-1],g_spng_IDAT_BUFFER_unfiltered[i]);
				j++;
			}
		}
		return;
	}

	if(g_spng_spnginf.clr == 6){
		for(int i = 3; i< g_spng_spnginf.width*g_spng_spnginf.height*4;i+=4){
			if(withAlpha){
				(*pixelbuffer)[j-1] = spng_calculate_greyscale(g_spng_IDAT_BUFFER_unfiltered[i-3],g_spng_IDAT_BUFFER_unfiltered[i-2],g_spng_IDAT_BUFFER_unfiltered[i-1]);
				(*pixelbuffer)[j] = g_spng_IDAT_BUFFER_unfiltered[i];
				j+=2;
			}
			else{
				(*pixelbuffer)[j] = spng_calculate_greyscale(g_spng_IDAT_BUFFER_unfiltered[i-3],g_spng_IDAT_BUFFER_unfiltered[i-2],g_spng_IDAT_BUFFER_unfiltered[i-1]);
				j++;
			}
		}
		return;
	}
	if(g_spng_spnginf.clr == 4 && withAlpha == 0){
		for(int i = 1; i< g_spng_spnginf.width*g_spng_spnginf.height*2;i+=2){
				(*pixelbuffer)[j] = g_spng_IDAT_BUFFER_unfiltered[i-1];
				(*pixelbuffer)[j] = g_spng_IDAT_BUFFER_unfiltered[i-1];
				(*pixelbuffer)[j] = g_spng_IDAT_BUFFER_unfiltered[i-1];
				j++;
		}
		return;
	}
}

int SPNG_read(char * filename,struct SPNG_INFO* spnginf){
	g_spng_is_little_endian = spng_cpu_get_is_little_endian();
	if(filename == NULL) return SPNG_FILE_NOT_FOUND;
	FILE * fp = fopen(filename,"rb");
	if(fp == NULL)return SPNG_FILE_NOT_FOUND;
	if(g_spng_is_allocated==1) free(g_spng_IDAT_BUFFER_unfiltered);
	int inf = SPNG_get_spnginfo_from_file(filename,&g_spng_spnginf);
	if(inf < 0) return inf;
	spng_read_IDAT(fp, &g_spng_spnginf, inf);
	spng_undo_filters(&g_spng_spnginf);
	if(g_spng_spnginf.clr == 3){
		if(g_spng_plte_is_allocated == 1) free(g_spng_plte_pixels);
		spng_get_plte(filename,inf);
	}
	g_spng_is_allocated=1;
	SPNG_get_spnginfo(spnginf); 
	fclose(fp);
		


	return SPNG_SUCCESS;
}
