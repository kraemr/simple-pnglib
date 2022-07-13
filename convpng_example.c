

#include "pnglib.h"
#include <string.h>

void printhelp(){
    printf("\033[0;31m");
    printf("\nUsage:\npngconv filename newfilename clrout\n\n");
    printf("\nclr can be -rgba -rgb -g or -ga");
    printf("\nrgba = red green blue alpha ");
    printf("\nrgb = red green blue");
    printf("\ngr = greyscale ");
    printf("\ngra = greyscale alpha\n");

}
int main(int argc,char* argv[]){
    
    uint8_t* pixelvalues = NULL;
    PNG_INFO pnginfo;

    // argv[1] is the the png to be converted
    if(argv[1] == NULL){
        printhelp();
        return 1;
    }
    // argv[2] is the new converted png
    if(argv[2] == NULL){
        printhelp();
        return 1;
    }
    // argv[3] is the converted png's colortype'
    if(argv[3] == NULL){
        printhelp();
        return 1;
    }
    char* opt = argv[3];
    unsigned char clr_out = 0;

    if(!strcmp(opt,"-g")){
        clr_out = Greyscale;
    }
    else if(!strcmp(opt,"-ga")){
        clr_out = Greyscale_Alpha;
    }
    else if(!strcmp(opt,"-rgb")){
        clr_out = Truecolour;
    }
    else if(!strcmp(opt,"-rgba")){
        clr_out = Truecolour_Alpha;
    }
    else if(!strcmp(opt,"-i")){
        clr_out = Indexed_colour;
    }
    else{
        printhelp();
        return 1;
    }

    PNG_Init(argv[1]); // Initializes the pixel buffer and loads the values 
    PNG_Get_PNGINFO(&pnginfo); // Get the dimensions (width height) colortype and other info
    PNG_Get_Pixelvals_RGBA(&pixelvalues); // Extracts the values out of the internal buffers and convert them to RGBA
  
    
    //note: You can also get the raw values with PNG_Get_Pixelvals();

    PNG_Exit();//Deallocates ALL internal buffers Extract values before you use this
    
    Png_Encode(pixelvalues,argv[2],pnginfo.width,pnginfo.height,clr_out,6,8); // This function expects RGBA input for now
    
    free(pixelvalues);//fre

}
