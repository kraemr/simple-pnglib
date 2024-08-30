# simple-pnglib
simple png library in pure c 

## Compilation
```bash
#in simple-pnglib
make #compiles c code into .so file
cd examples
./compile.sh
```
## How to use:
After Compilation what you need to do to use the library is copy spnglib.h and spnglib.so to somewhere in your project.
Then when you compile your own project you have to link against spnglib.so.

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


