#include "../include/spnglib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char * argv[]){
    char out_filename[128]= {0}; // init with zeroes
    char in_filename[128] = {0};
    if(argc >= 3){
        memcpy(out_filename,argv[2],128);
        memcpy(in_filename,argv[1],128);
        struct SPNG_INFO spnginf;
        int res = SPNG_read(in_filename,&spnginf);
        unsigned char * pixelbuf;
        // This will overwrite the values in spnginf with the ones for greyscale
        // For now any call to get_pixels functions allocates the pixelbuf for you
        // Which is why you do not need to allocate it
        SPNG_get_pixels_greyscale(&spnginf,&pixelbuf, 0);
        FILE* fp = fopen(out_filename,"wb");
        SPNG_write_metadata(fp,&spnginf); // write IHDR(Metadata width height ...)  // THIS IS NEEDED TO MAKE THE IMAGE INTO A VALID PNG!!!!
        SPNG_write(fp, &spnginf, pixelbuf);
        fclose(fp);
        free(pixelbuf);
        SPNG_exit(); // Call Exit to deallocate any internal buffers
    }
    else{
        printf("Invalid Arguments ,Usage: ./rgb2grey in.png out.png");
    }
    return 0;
}