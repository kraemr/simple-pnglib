#include "../include/spnglib.h"
int main(int argc,char* argv[]){
    if(argc < 2){
        printf("Invalid Arguments, Usage: pnginfo filename.png");
        return 1;
    }
    struct SPNG_INFO spnginf;
    SPNG_get_spnginfo_from_file(argv[1], &spnginf);
    printf("bytes per pixel: %d\n",spnginf.bytespp);
    printf("bitdepth: %d\n",spnginf.bitdepth);
    printf("color type: %d\n",spnginf.clr);
    printf("width: %d\n",spnginf.width);            
    printf("height: %d\n",spnginf.height);
    printf("size: %ld bytes\n",spnginf.size);




}