#include "pnglib.h"
int w = 0;
int h = 0;

int main(int argc, char* argv[]){
 
	if(argv[1] == NULL){
	perror("Wrong usage");
	exit(1);
	}
	if(argv[2] == NULL){
	perror("Wrong usage");
	exit(1);
	}
	if(argv[3] == NULL){
	exit(1);
	}
  if(argv[4] == NULL){
	exit(1);
	}

   PNG_Init(argv[1]);
   
	char* new_fname=argv[2];
    char scale_arg = argv[3][0];
    int scale = scale_arg-48; // convert to number
    printf("scale is: %d",scale);
    PNG_Get_Dim(&w,&h);
    printf("w:%d h:%d  ",w,h);
    pixel_pp pxpp ;
    uint8_t* pp;
    //PNG_Get_Pixelpp(&pxpp);
     //printf("%u",pxpp[2000][1000].b);
     //PNG_Free_2dpixel(h,&pxpp);
    //PNG_Get_Pixelvals_RGBA(&pp);
    PNG_Get_Pixelvals_RGBA(&pp);
    PNG_Get_Pixelpp(&pxpp,pp,6);
    //PNG_Get_Pixelvals(&pp);
    PNG_Free_2dpixel(h,&pxpp);
  //  uint8_t* tt;
//    mipmap(&pp,&tt,h,w,scale);
    Png_Encode(pp,new_fname,w,h,argv[3][0]-48,argv[4][0]-48,argv[5][0]-48);
    PNG_Exit();

    //printf("%d",pp[11110]);
    free(pp);


}
