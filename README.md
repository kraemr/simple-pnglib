# simple-pnglib
simple png library in pure c 


## How to use:
```c
#include "include/spnglib.h"
int main(){
        unsigned char * px;
        struct SPNG_INFO spnginf; // contains width height colortype bytespp ...
        SPNG_read("test.png",&spnginf);
        SPNG_get_pixels(&px); // read from internal buffer to px buffer
        SPNG_exit(); // deallocate the internal buffer
        FILE * fp = fopen("res.png","wb"); // create file pointer for image
        SPNG_write_metadata(fp,&spnginf); // write IHDR(Metadata width height ...)  // THIS IS NEEDED TO MAKE THE IMAGE INTO A VALID PNG!!!!
        SPNG_write(fp,&spnginf,px); // Finally write the IDAT portion of the image, ie the pixels
        fclose(fp);
}
```
