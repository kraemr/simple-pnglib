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
use make in the simple-pnglib folder to create the .dll, the youll also need to copy the header files in include to your own include location.
The only dependency is zlib, this should be available on most systems.
You will need a zlib dll or a statically compiled zlib to link against.
