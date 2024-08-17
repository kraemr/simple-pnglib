#include "../include/spnglib.h"
#include <stdlib.h>
#include <stdio.h>

// This converts this executable to a .png file
int main(int argc, char * argv[]){
        if(argc < 2){
                printf("Invalid Argument count\n");
                return 1;
        }
        const unsigned int BUF_SIZE = 16384;
	unsigned char * px = malloc(3*1024 * 1024);
        unsigned char file_buf[BUF_SIZE];
        FILE * executable_fp = fopen(argv[1],"rb");
        if(executable_fp == NULL){
                printf("File '%s' does not exist\n",argv[1]);
                return 1;
        }
        fseek(executable_fp,0,SEEK_END);
        long long unsigned int len = ftell(executable_fp);
        fseek(executable_fp,0,SEEK_SET);
        long long unsigned int remaining_bytes=len;
        long long unsigned int px_i = 0;
        int t = 0;
        
        while(!feof(executable_fp)){
                printf("%llu %llu\n",remaining_bytes,px_i);
                remaining_bytes -= 16384;
                fread(file_buf,BUF_SIZE,1,executable_fp);
                if(remaining_bytes < 16384){
                        t = remaining_bytes;
                }else{
                        t = 16384;
                }
                for(int i = 0; i < t; i+=1){
                        px[px_i] = file_buf[i];
                        px_i++;
                }
        }
        printf(" done %llu \n",px_i);
        
        struct SPNG_INFO spnginf; // contains width height colortype bytespp ...
        spnginf.width = 128;
        spnginf.height = 128;
        spnginf.clr = 2;
        spnginf.bitdepth = 8;
        spnginf.bytespp = 3;
        
        printf("%d %d %d %d %d",spnginf.width,spnginf.height,spnginf.clr,spnginf.bitdepth,spnginf.bytespp,spnginf.size);
	FILE * fp1 = fopen("res.png","wb"); // create file pointer for image
	SPNG_write_metadata(fp1,&spnginf); // write IHDR(Metadata width height ...)  // THIS IS NEEDED TO MAKE THE IMAGE INTO A VALID PNG!!!!
        SPNG_write(fp1,&spnginf,px); // Finally write the IDAT portion of the image, ie the pixels
        fclose(fp1);
        fclose(executable_fp);
        free(px);//And of course free this after
}
