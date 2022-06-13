#include "pnglib.h"

typedef char* String;

int w = 0;
int h = 0;

int test(String* t){
*t = malloc(10); 
char f = 'h';
*t[0] = f;
}



int main(int argc, char* argv[]){
    PNG_Init(argv[1],2);
    PNG_Get_Dim(&w,&h);
    printf("w:%d h:%d  ",w,h);
    pixel_pp pxpp ;
    uint8_t* pp;
    PNG_Get_Pixelpp(&pxpp);
     printf("%u",pxpp[2000][1000].b);
     PNG_Free_2dpixel(h,&pxpp);
    PNG_Get_Pixelvals(&pp);
     //PngEncode();
     PNG_Exit();

     Png_Encode(pp,"hey.png",w,h,4);

}