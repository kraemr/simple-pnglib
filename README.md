# simple-pnglib
simple png library in pure c 


## How to use:
```c
#include "include/spnglib.h"
#include <stdlib.h>
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
        free(px);//And of course free this after
}
```
## Compilation
``` bash
gcc -c spnglib_write.c -o spnglib_write.o
gcc -c spnglib_write_txt.c -o spnglib_write_txt.o
gcc -c spnglib_read.c -o spnglib_read.o
gcc -c spnglib_read_txt.c -o spnglib_read_txt.o
# Then link these with your program
# you could also build yourself a dll
# you could also just include them directly:
gcc yourprogram.c spnglib_read.c spnglib_write.c ... -lz
```
The only dependency is zlib, this should be available on most systems.
