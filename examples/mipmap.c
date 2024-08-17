//Tool to scale up or down images 2x,4x ...
#include "../include/spnglib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isPower(int x, long int y)
{
    if (x == 1)
        return (y == 1); 
    long int pow = 1;
    while (pow < y)
        pow *= x;
    return (pow == y);
}

//new_buf should be 3x smaller if ratio is 3
void downscale(struct SPNG_INFO* spnginf,unsigned char* orig_buffer,unsigned char* new_buf,unsigned int ratio){
    unsigned char * memcp_buf_pointer  = new_buf;
    int pixel_i = 0;
    int line_i = ratio-1;    
    unsigned int scanline_len = spnginf->width * spnginf->bytespp; // bytes per line in the image
    for(long i = 0; i < spnginf->size; i+=scanline_len){
        for(long j =i;j < i+scanline_len; j+=spnginf->bytespp){
            if(pixel_i % ratio == 0){
                memcpy(memcp_buf_pointer,&orig_buffer[j],spnginf->bytespp);// not very efficient to copy a few bytes ...
                memcp_buf_pointer += spnginf->bytespp; // move pointer forward by the bytes per pixel
            }
            pixel_i++;
        }
        line_i = ratio-1;
        while(line_i){
            i += scanline_len;
            line_i--;
        }
    }
    spnginf->width /= ratio;
    spnginf->height /= ratio;
    spnginf->size = spnginf->width * spnginf->height * spnginf->bytespp;
}

//new_buf should be 3x bigger if ratio is 3
void upscale(struct SPNG_INFO* spnginf,unsigned char* orig_buffer,unsigned char* new_buf,unsigned int ratio){
    unsigned char * memcp_buf_pointer  = new_buf;
    int line_i = ratio;
    int ratio_i = ratio-1;
    
    unsigned int scanline_len = spnginf->width * spnginf->bytespp;
    unsigned int new_scanline_len = spnginf->width * ratio * spnginf->bytespp;

    for(long i = scanline_len; i < spnginf->size; i+=scanline_len){
        for(long j =i-scanline_len;j < i;j+=spnginf->bytespp){
            ratio_i = ratio;  
            while(ratio_i){ // construct an upscaled vertical line 
                memcpy(memcp_buf_pointer,&orig_buffer[j],spnginf->bytespp);// not very efficient to copy a few bytes ...
                memcp_buf_pointer += spnginf->bytespp;
                ratio_i--;
            }
        }
        ratio_i = ratio-1;  
        while(ratio_i){
            memcpy(memcp_buf_pointer, memcp_buf_pointer-new_scanline_len,new_scanline_len);
            memcp_buf_pointer += new_scanline_len;
            ratio_i--;
        }

    }   
    spnginf->width *= ratio;
    spnginf->height *= ratio;
    spnginf->size = spnginf->width * spnginf->height * spnginf->bytespp;
}



int main(int argc, char* argv[]){
    char out_filename[128]= {0}; // init with zeroes
    char in_filename[128] = {0};
    if(argc >= 5){
        memcpy(out_filename,argv[2],128);
        memcpy(in_filename,argv[1],128);
        struct SPNG_INFO spnginf;
        unsigned char * pixelbuf;
        unsigned char * new_buf;
        int ratio=2;
        int downscale_image= argv[4][0] == 'd' ? 1 : 0;
        ratio = atoi(argv[3]);
        
        if(ratio < 0){
            ratio *= -1; //only positive
        }
        if( isPower(2,ratio) == 0 ){
            printf("ratio: %d is NOT a power of 2 exiting",(int)sqrt(ratio));
            return 1;
        }

        int res = SPNG_read(in_filename,&spnginf);
        SPNG_get_pixels(&pixelbuf);     

        if(downscale_image){
            new_buf = malloc((spnginf.width/ratio) * (spnginf.height/ratio) * spnginf.bytespp);
            downscale(&spnginf,pixelbuf,new_buf,ratio);
        }else{
            new_buf = malloc((spnginf.width*ratio) * (spnginf.height*ratio) * spnginf.bytespp);
            upscale(&spnginf,pixelbuf,new_buf,ratio);
        }

        FILE* fp = fopen(out_filename,"wb");
        SPNG_write_metadata(fp,&spnginf); // write IHDR(Metadata width height ...)  // THIS IS NEEDED TO MAKE THE IMAGE INTO A VALID PNG!!!!
        SPNG_write(fp, &spnginf, new_buf);
        fclose(fp);
        SPNG_exit(); // Call Exit to deallocate any internal buffers
        free(new_buf);
        free(pixelbuf); // we dont need it anymore
    }
    else{
        printf("Invalid Arguments ,Usage: ./mipmap in.png out.png 2 d \n Where 2 is the ratio to scale by, and d means downscale, where u would upscale\n");
    }
}