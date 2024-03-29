#include "include/spnglib.h"
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char * argv[]){
        const unsigned int BUF_SIZE = 16384;
	unsigned char * px = malloc(3*1024 * 1024);
        unsigned char file_buf[BUF_SIZE];
        FILE * executable_fp = fopen(argv[1],"rb");
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


        // Add the init function seperately
      //  SPNG_read("test.png", &spnginf);
        
        spnginf.width = 256;
        spnginf.height = 256;
        spnginf.clr = 0;
        spnginf.bitdepth = 8;
        spnginf.bytespp = 1;
        
        printf("%d %d %d %d %d",spnginf.width,spnginf.height,spnginf.clr,spnginf.bitdepth,spnginf.bytespp,spnginf.size);
        //SPNG_get_pixels(&px); // read from internal buffer to px buffer
        //SPNG_exit(); // deallocate the internal buffer
        
	FILE * fp1 = fopen("res.png","wb"); // create file pointer for image
        
	SPNG_write_metadata(fp1,&spnginf); // write IHDR(Metadata width height ...)  // THIS IS NEEDED TO MAKE THE IMAGE INTO A VALID PNG!!!!
        
        SPNG_write(fp1,&spnginf,px); // Finally write the IDAT portion of the image, ie the pixels
        
        fclose(executable_fp);
        free(px);//And of course free this after
}
