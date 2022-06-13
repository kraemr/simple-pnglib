#include "pnglib.h"

//******** Variable Definitions ********//{
struct pixel** pixel_rows; //TODO: Remove this later out of .h
unsigned int Little_Endian; // if 1 System is little endian else Big endian
int bytes_pp=0;
ssize_t PNG_file_size=0; // FILE SIZE IN BYTES
unsigned char* IDAT_Buffer;
unsigned char* IDAT_Buffer2;

struct IHDR_CHUNK ihdr_chunk;
struct PLTE_CHUNK plte_chunk;
struct TRNS_CHUNK trns_chunk;
struct z_stream_s zst;
struct pixel** pixel_rows;
unsigned long IDAT_Length=0;

#define Greyscale 0
#define Truecolour 2 
#define Indexed_colour 3 
#define Greyscale_Alpha 4 
#define Truecolour_Alpha 6 

#define NOFILTER 0
#define SUB 1
#define UP 2
#define AVERAGE 3
#define PAETH 4

int Optionsflag=0; 
// 0 = only IDATbuffer
// 1 = only 2d idat
// 2 = only pixel_rows
// 3 = IDATBUFFER and pixel_rows
// 4 = 2d idat and pixel_rows
// 5 = pixel_rows and idat
// 6 = IDATBUFFER, pixel_rows and 2d idat


const uint8_t png_identifier[]={
    0x89,0x50,0x4e,0x47,0xd,0xa,0x1a,0xa
};
const uint8_t standard_ihdr_length[]={
    0x00,0x00,0x00,0x0d
};
const uint8_t IDAT_ID[]={
    0x49,0x44,0x41,0x54
};
const uint8_t IDAT_END_ID[]={
    0x49,0x45,0x4e ,0x44
};
const uint8_t PLTE_identifier[]={
    0x50,0x4c,0x54,0x45   
};
const uint8_t IHDR_identifier[]={
    0x49,0x48,0x44,0x52
};
const uint8_t Trns_id[]={
    0x74,0x52 ,0x4E ,0x53
};
const uint8_t zeroes[] ={0,0,0,0};

//******** Variable Definitions ********//}

/*Pass in your width and height variables as reference
Because this function will write width and height to these vars
*/
void PNG_Get_Dim(int* w, int* h){
    *w = ihdr_chunk.width;
    *h = ihdr_chunk.height;
}

void PNG_Get_Pixelpp(pixel_pp* pp){
    
    long col_s = ihdr_chunk.height * sizeof(struct pixel*);
    (*pp) = malloc(col_s);
    long row_s = ihdr_chunk.width * sizeof(struct pixel);
   
    for(int i= 0; i < ihdr_chunk.height; i++){
       (*pp)[i] = malloc(row_s);
    }
    
    for(int i= 0; i < ihdr_chunk.height; i++){
    
        for(int j= 0; j < ihdr_chunk.width; j++){
            (*pp)[i][j] = pixel_rows[i][j] ; 
        }
        
    }

}
void PNG_Clean(){

}

uint8_t* ScanLine;
uint8_t* previous_ScanLine;
int ScanLineLength=0;

void get_last_block(uint8_t* lastblock,uint8_t* Scanline,int pos){
    int index = 0;
    for(int i = pos; i<pos+bytes_pp;i++){
        lastblock[index] = Scanline[i];
       // ////printf("lastblock %d   %d at %d\n ",lastblock[index],index, i);
        index++;
    }
}
void get_current_block(uint8_t* currblock,uint8_t* Scanline,int pos){
    int index = 0;
    for(int i = pos; i<pos+bytes_pp;i++){
        currblock[index] = Scanline[i];
       // ////printf("currblock %d   %d at %d\n ",currblock[index],index,i);
        index++;
    }
}
void Add_blocks(uint8_t* lastblock,uint8_t* currblock,uint8_t* Scanline,int pos){
    for(int i = 0 ; i< bytes_pp; i++){
        currblock[i]+=lastblock[i];
        Scanline[pos] = currblock[i];
       // ////printf("Scanline[%d] is now  %d",pos,currblock[i]);
        pos++;
    }
 
}

void Undo_Sub(uint8_t* ScanLine, int ScanLineLength){
    uint8_t* lastblock = malloc(bytes_pp);
    uint8_t* currblock = malloc(bytes_pp);
    int first = 1;
    //////printf("\n");
    for(int i = 0 ; i < ScanLineLength-bytes_pp; i+=bytes_pp){
                //////printf("i: %d",i);
                if(first){
                get_last_block(lastblock,ScanLine,i);
                get_current_block(lastblock,ScanLine,i);
                first = 0;
                }
                else{
                get_last_block(lastblock,ScanLine,i-bytes_pp);
                get_current_block(currblock,ScanLine,i);
                Add_blocks(lastblock,currblock,ScanLine,i);
                }
            
            //////printf(" %u ",ScanLine[i]);
    }
    free(lastblock);
    free(currblock);
}

void zero_block(uint8_t* block){
    for(int i=0;i<bytes_pp;i++){
        block[i]=0;
    }
}

void get_last_Line(int pos,int ScanLineLength,uint8_t* prev_scan){
    //printf("\n scan end: %d",pos);
    pos++;
    pos -= ScanLineLength;
    //printf(" %u ",IDAT_Buffer2[pos-1]);
    //printf("\n scan start: %d",pos);

    int i;
    for(int j = pos; j <pos+ScanLineLength-1; j++)
    {
       // prev_scan[i] = IDAT_Buffer2[j];
        ////printf(" prev Scanline: %u at %d",prev_scan[j-1-pos],j);
        //  free(b);
        i++;
        }
}

//gets the last Scanline
void Undo_Up(int ScanLineLength){
  for(int i = 0;i<ScanLineLength-1;i++){
      ScanLine[i]+=previous_ScanLine[i];
  }
}

void Undo_Avg(){
    unsigned int a = 0;
    unsigned int b = 0;
    uint8_t* last_block=malloc(bytes_pp);
    uint8_t* curr_block=malloc(bytes_pp);    
    for(int i = 0 ; i < ScanLineLength-bytes_pp; i+=bytes_pp){
       if(i==0){
                get_last_block(last_block,ScanLine,i);
                get_current_block(curr_block,previous_ScanLine,i);
                for(int j = i; j < bytes_pp+i; j++){
                    a = 0;
                    b = curr_block[j-i];
                    ScanLine[j] += ((a + b) /2) ;
                
                    ////printf("Flooring a:%u and b:%u = %u",a,b,ScanLine[j]);
                }
                }
                else{
                get_last_block(last_block,ScanLine,i-bytes_pp);
                get_current_block(curr_block,previous_ScanLine,i);

                for(int j = i; j < bytes_pp+i; j++){
                    a = last_block[j-i];
                    b = curr_block[j-i];
                    ScanLine[j]+=  ((a + b) /2);
                   // //printf("Flooring a:%u and b:%u = %u",a,b,ScanLine[j]);
                }
                }
    }
    free(last_block);
    free(curr_block);
}

/*
    x = current_byte
    a = byte before current byte
    b = byte at x's position in the last line
    c = byte preceding b 
*/
uint8_t PaethPredictor(uint8_t a,  uint8_t b, uint8_t c){
    int  p =a + b - c;
    int pa = abs(p - a);
    int  pb = abs(p - b);
    int pc = abs(p - c);
    uint8_t PR=0; // predictor
    if(pa <= pb && pa <= pc){
        PR = a;
    }
    else if(pb <= pc){
        PR = b;
    }
    else{
        PR = c;
    }
    return PR;
}
void Undo_Paeth(){
    uint8_t* last = malloc(bytes_pp);
    uint8_t* current = malloc(bytes_pp);
    uint8_t* last_scan_curr = malloc(bytes_pp);
    uint8_t* last_scan_last = malloc(bytes_pp);

   uint8_t a=0;
   uint8_t b=0;
   uint8_t c=0;

   for(int i=0; i< ScanLineLength-bytes_pp;i+=bytes_pp){
         if(i==0){
                get_last_block(last,ScanLine,i);
                get_current_block(current,ScanLine,i);
                get_current_block(last_scan_curr,previous_ScanLine,i);
                get_last_block(last_scan_curr,previous_ScanLine,i);
                for(int j = i; j<bytes_pp+i;j++){
                   a = 0;
                   b = last_scan_curr[j-i];
                   c = 0;
                   ScanLine[j] += PaethPredictor(a,b,c);
                }
            }
        else{
                get_last_block(last,ScanLine,i-bytes_pp);
                get_current_block(current,ScanLine,i);
                get_current_block(last_scan_curr,previous_ScanLine,i);
                get_last_block(last_scan_last,previous_ScanLine,i-bytes_pp);
                for(int j = i; j<bytes_pp+i;j++){
                   a = last[j-i];
                   b = last_scan_curr[j-i];
                   c = last_scan_last[j-i];
                   ScanLine[j] += PaethPredictor(a,b,c);
                }
        }   
    }
    free(last);
    free(last_scan_last);
    free(last_scan_curr);
    free(current);
}

//get a reference to the IDAT_i 
int Copy_ScanLine(uint8_t* ScanLine,int ScanLineLength,int IDAT_i){
    for(int j = 0; j < ScanLineLength-1;j++){
        IDAT_Buffer[IDAT_i] = ScanLine[j];
        IDAT_i++;
    }  
    return IDAT_i;
}

//TODO: Implement Paeth and Average
void Undo_Filters(){
    ScanLineLength = (ihdr_chunk.width * bytes_pp) + 1;
    int BufferSize = ScanLineLength * ihdr_chunk.height;
    IDAT_Buffer = malloc(BufferSize);
    int IDAT_i = 0;
    previous_ScanLine = malloc(ScanLineLength -1);
    ScanLine = malloc(ScanLineLength -1);
    //printf("BufferSize %d ",BufferSize);
   // //printf("\n Scanline Addr: %p",&ScanLine);
    for(int i = 0;i< BufferSize;i+=ScanLineLength){               
                if(i == 0){
                    for(int j = i+1 ; j <ScanLineLength+i;j++)
                    {
                        previous_ScanLine[j-1-i] = 0;
                        ScanLine[j-1-i] = IDAT_Buffer2[j];
                    }
                }
                else{
                    for(int j = 0; j < ScanLineLength-1; j++){
                        previous_ScanLine[j] = ScanLine[j];
                    }
                    for(int j = i+1 ; j <ScanLineLength+i;j++)
                    {
                        ScanLine[j-1-i] = IDAT_Buffer2[j];
                    }
                }
                switch (IDAT_Buffer2[i])
                {
                case NOFILTER:
                    //printf("\n No Filter at %d",i);
                   IDAT_i = Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);
                ;break;

                case SUB:
                    //printf("\n Sub Filter a %d IDAT: %d",i,IDAT_i);
                    Undo_Sub(ScanLine,ScanLineLength);
                   IDAT_i = Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);
                    //printf("\n after IDAT: %d",IDAT_i);
                   ;break;
                case UP:
                    //printf("\n Up Filter at %d",i);
                    //get_last_Line(i,ScanLineLength,previous_ScanLine);
                    Undo_Up(ScanLineLength);
                    IDAT_i = Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);
                ;break;

                case AVERAGE:
                    //printf("\n Average Filter at %d",i);
                    Undo_Avg();
                    IDAT_i = Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);
                ;break;

                case PAETH:
                    //printf("\n Paeth Filter at %d",i);
                    Undo_Paeth();
                    IDAT_i = Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);
                ;break;

                default:
                   // printf("Unknown Filter at %d filter is:%d ",i,IDAT_Buffer2[i]);
                Copy_ScanLine(ScanLine,ScanLineLength,IDAT_i);;break;
                break;
                }
            } // for loop
            free(ScanLine);
            free(previous_ScanLine);
        }






//######## pngreading Functions #########//{

long long int get_file_size(char *pathname){
    struct stat file_stat;
    stat(pathname,&file_stat);
    return file_stat.st_size;
}

int File_Exists(char* file_name){
  if( access( file_name, F_OK ) != -1)return 1;
  else return 0;  
    //perror("file is not found");
}
int is_little_endian(){
    int num = 1;
    if (*(char *)&num == 1)return 1;//printf("Little-Endian\n");
    else return 0;//printf("Big-Endian\n");
}

void convert_big_to_little_endian(unsigned int* n){
*n = (*n >> 24) | ((*n >> 8) & 0x0000ff00) | ((*n << 8) & 0x00ff0000) | (*n << 24);
}

unsigned int compare_bytes_with_id(FILE* fp,unsigned char* buf,const uint8_t id[4]){
    if(fp == NULL){
        for(int i = 0 ; i< 4;i++){
            ////////printf(" %u vs %u",buf[i],id[i]);
            if(buf[i] != id[i]){
                return 0;
            }
        }
    }
    else{
        unsigned char* temp;
        temp = malloc(4);
        fread(temp,1,4,fp);
        for(int i=0;i<4;i++){
            if(temp[i] != id[i]){
              //  //////printf(" %u not matching id %u\n",temp[i],id[i]);
                free(temp);
                return 0;
            } 
        }
        free(temp);
    }
    return 1; 
}

unsigned char* IDAT_Buffer;


unsigned int read_chunk_length(FILE* fp){
    unsigned char* temp=malloc(4);
    fseek(fp,-8,SEEK_CUR); // seek to the start of the lengths
    fread(temp,1,4,fp);
    unsigned int length=*(unsigned int*)(temp);

    if(Little_Endian){
        fseek(fp,4,SEEK_CUR);
        free(temp);
        convert_big_to_little_endian(&length);
        return length;
    }
    else{
        fseek(fp,4,SEEK_CUR);
        free(temp);
        return length;
    }
    free(temp);
}

void get_IDAT(FILE* fp){
    IDAT_Buffer = malloc(PNG_file_size  );
    //fseek(fp,-1,SEEK_CUR);
    //first we read 4 bytes into buffer and compare then if it is IDAT count the values one by one 
    unsigned char* temp=malloc(4);
    int IDAT_i=0;
    unsigned int chnk_length=0; // chunk length
    unsigned int i=0;
    unsigned int IDAT_COUNT=0;

    while(!feof(fp)){
        fread(temp,1,1,fp);
        if(temp[0] == IDAT_ID[0]){
            fseek(fp,-1,SEEK_CUR);
            fread(temp,1,4,fp);

            if(compare_bytes_with_id(NULL,temp,IDAT_ID)){
                i = 0; IDAT_COUNT++; 
             //   //////printf("found IDAT\n");
                chnk_length = read_chunk_length(fp);
                //printf("chnkk len: %d",chnk_length);
                while(i < chnk_length) {
                    fread(temp,1,1,fp);
                    IDAT_Buffer[IDAT_i] = temp[0];
                    IDAT_i++;
                    i++;
       
                }
              //  //////printf(" IDAT: %u temp: %x IDAT_i %d",IDAT_Buffer[IDAT_i],temp[0],IDAT_i);
            }
            else if(compare_bytes_with_id(NULL,temp,IDAT_END_ID)){
            //    //////printf("Found IDAT_END\n");
                //realloc(IDAT_Buffer,IDAT_i);
                IDAT_Length = IDAT_i;
                ////printf("IDAT Length: %u",IDAT_Length);
                break;
            }
        }
    //Puts all the IDAT DATA in an 
    }
    //////printf("found %d IDAT CHUNKS", IDAT_COUNT);
    free(temp);
}



void print_decoded(){
    for(int i = 0; i<ihdr_chunk.width * ihdr_chunk.height * ihdr_chunk.bit_depth;i++ ){
        //printf(" %u ",IDAT_Buffer2[i]);
    }
}

//decompresses the idat buffer with inflate()
void decode_IDAT(){
    IDAT_Buffer2=malloc(ihdr_chunk.width * ihdr_chunk.height * ihdr_chunk.bit_depth);
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = IDAT_Length; // size of input
    infstream.next_in = (Bytef *)IDAT_Buffer; // input char array
    if(ihdr_chunk.colour_type != 3){
    infstream.avail_out = ihdr_chunk.width * ihdr_chunk.height * ihdr_chunk.bit_depth ; // size of output
    }
    else{
        infstream.avail_out = ihdr_chunk.width * ihdr_chunk.height * ihdr_chunk.bit_depth ; // size of output
    }
    infstream.next_out = (Bytef *)IDAT_Buffer2; // output char array
    int e = 0;

    inflateInit(&infstream);
    e = inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    //////printf("inflate return %d",e);
    free(IDAT_Buffer);//free it here since it gets allocated later on. To avoid MemLeak
    print_decoded();
}



void get_PLTE(FILE* fp){
    unsigned char* b = malloc(4);
    while(!feof(fp)){
        fread(b,1,1,fp);
        if(b[0] == PLTE_identifier[0]){
            fseek(fp,-1,1); //go back one to read the whole thing
            fread(b,1,4,fp);
            if(compare_bytes_with_id(NULL,b,PLTE_identifier)){ // check if the 4 bytes match the identifier
                fseek(fp,-8,SEEK_CUR); // go back to the lengths
                fread(b,1,4,fp); // read the length
                plte_chunk.length = *(unsigned int*)(b);
                if(Little_Endian == 1)convert_big_to_little_endian(&plte_chunk.length); // get the chunk length
                ////////printf("plte chunk len: %u ",plte_chunk.length);
                fseek(fp,4,SEEK_CUR);      
                break;
            }
        }
    }
    plte_chunk.length % 4 == 0 ? 0 /* is divisible by three */ : perror("Plte Chunk Length is NOT divisble by three");
    free(b);
    b= malloc(4 * sizeof(unsigned char));
    plte_chunk.Chunk_Data = malloc(plte_chunk.length * sizeof(unsigned int) );
    for(int i=0;i<plte_chunk.length;i++){
        fread(b,1,1,fp);
        plte_chunk.Chunk_Data[i] = b[0];
    }
    fread(b,1,4,fp);
    plte_chunk.CRC = *(unsigned int*)(b);
    if(Little_Endian == 1)convert_big_to_little_endian(&plte_chunk.CRC);
    free(b);
}

int is_IHDR(FILE* fp){
    unsigned char* b = malloc(4);
   
    fseek(fp,4,SEEK_CUR);
        fread(b,1,4,fp);
        for(int i=0;i<4;i++){
            
            if(b[i] != IHDR_identifier[i]){
                free(b);
                return 0;
            }
            ////////printf(" %x vs %x , ",b[i] , IHDR_identifier[i] );
        }
        free(b);
        return 1;
}

void set_Bytes_per_pixel(){
        switch (ihdr_chunk.colour_type)
        {
        case Greyscale: bytes_pp = (1 * ihdr_chunk.bit_depth) / 8 ;break;
        case Greyscale_Alpha: bytes_pp = (2 * ihdr_chunk.bit_depth) / 8; break;
        case Truecolour_Alpha: bytes_pp = (4 * ihdr_chunk.bit_depth) / 8; break;
        case Truecolour: bytes_pp = (3 * ihdr_chunk.bit_depth) / 8; break;
        case Indexed_colour: bytes_pp = (4 * ihdr_chunk.bit_depth) / 8; break;        
        default:
            break;
        }
}

void Print_trns(){
    for(int i = 0; i< trns_chunk.length; i++){
        ////printf("%u ",trns_chunk.Chunk_Data[i]);
    }
}

struct pixel* plte_pixels;
void Print_PLTE_Pixel(){
    for(int i = 0; i< plte_chunk.length / 3 ; i++){
      //  ////printf("\n %u %u %u",plte_pixels[i].r,plte_pixels[i].g,plte_pixels[i].b);
    }
}
void Convert_PLTE_To_Pixel(){
    plte_pixels =(struct pixel*)malloc(plte_chunk.length/3 * sizeof(struct pixel));
    int plte_i = 0;
    for(int i =2;i<plte_chunk.length;i+=3){
        plte_pixels[plte_i].r = plte_chunk.Chunk_Data[i-2];
        plte_pixels[plte_i].g = plte_chunk.Chunk_Data[i-1];
        plte_pixels[plte_i].b = plte_chunk.Chunk_Data[i];
        if(plte_i < trns_chunk.length){
            plte_pixels[plte_i].A = trns_chunk.Chunk_Data[plte_i];
        }
        else{
            plte_pixels[plte_i].A = 255;
        }
        plte_i++;
    }
    Print_PLTE_Pixel();
}

//Creates IDATbuffer from PLTE
void Make_IDATBUF_Plte( ){
    int ScanLen = (ihdr_chunk.width * 4)+1;
    IDAT_Buffer = malloc(ihdr_chunk.height * ihdr_chunk.width * 4);
    uint32_t IDAT_i= 0;
    uint32_t IDAT2_i= 0;
    uint8_t plte_i = 0;

    for(int i =0 ; i < ihdr_chunk.height; i++){
        for(int j = 0; j < ihdr_chunk.width;j++){
            j == 0 ? IDAT_i++: 0 ;
            plte_i = IDAT_Buffer2[IDAT_i];
            
            IDAT_Buffer[IDAT2_i] = plte_pixels[plte_i].r;
            IDAT2_i++;
            IDAT_Buffer[IDAT2_i] = plte_pixels[plte_i].g;
            IDAT2_i++;
            IDAT_Buffer[IDAT2_i] = plte_pixels[plte_i].b;
            IDAT2_i++;
            IDAT_Buffer[IDAT2_i] = plte_pixels[plte_i].A;
            IDAT2_i++;
            IDAT_i++;
        }   
    }
   printf(" IDATBUF len: %u h: %u w: %u bpp: %u ",IDAT_i,ihdr_chunk.height,ihdr_chunk.width,bytes_pp);

}

//this prepares the pixel struct accessed by the end user
void Make_2D_pixel_struct_PLTE(){
    pixel_rows = (struct pixel**)malloc(ihdr_chunk.height*sizeof(struct pixel*));
    for(int i = 0 ; i < ihdr_chunk.height; i++ ){
        pixel_rows[i] = (struct pixel*)malloc(ihdr_chunk.width*sizeof(struct pixel));   
    }
    int IDAT_i = 0;
    uint8_t plte_i = 0;
    for(int i = 0; i< ihdr_chunk.height; i++){

        for(int j=0; j< ihdr_chunk.width;j++){
            if(j == 0){
                IDAT_i++;
            }
        plte_i = IDAT_Buffer2[IDAT_i];
        pixel_rows[i][j] = plte_pixels[plte_i];
        IDAT_i++;
        }
    }
}

void Print_PLTE_2d(){
    int plte_i = 0;
     for(unsigned int i = 0; i< ihdr_chunk.height; i++){
        for(unsigned int j=0; j< ihdr_chunk.width;j++ ){
           // ////printf("\n y:%u x:%u %u %u %u ",i,j,pixel_rows[i][j].r,pixel_rows[i][j].g,pixel_rows[i][j].b);
            plte_i++;
        }
    }
}

void Print_PLTE(){
    for(int i = 0; i < plte_chunk.length;i++){
        if(i % 3 == 0){
            ////printf("\n");
        }
       // ////printf(" %u",plte_chunk.Chunk_Data[i]);
    }
}

int decode_IHDR(FILE* fp){
    if(is_IHDR(fp) ){
        unsigned char* b = malloc(4 * sizeof(unsigned char)); // assign 4 bytes
        fread(b,1,4,fp);
        ihdr_chunk.width = *(unsigned int*)(b);
        Little_Endian & 1 ?convert_big_to_little_endian(&ihdr_chunk.width):0;
        printf("\npng widht: %u ", ihdr_chunk.width);

        fread(b,1,4,fp);
        ihdr_chunk.height = *(unsigned int*)(b);
        Little_Endian & 1 ?convert_big_to_little_endian(&ihdr_chunk.height):0;
        printf("\npng height: %u ", ihdr_chunk.height);       // //////printf(" %d",foo);

        fread(b,1,1,fp);
        ihdr_chunk.bit_depth =  b[0];
        printf("\npng bit_depth: %u ", ihdr_chunk.bit_depth);

        fread(b,1,1,fp);
        ihdr_chunk.colour_type = b[0];
        printf("\npng clr type: %u ", ihdr_chunk.colour_type);

        fread(b,1,1,fp);
        ihdr_chunk.compression_method = b[0];
        printf("\npng compression type: %u ", ihdr_chunk.compression_method);        
        
        fread(b,1,1,fp);
        ihdr_chunk.Filter_method = b[0];
        printf("\npng filter type: %u ", ihdr_chunk.Filter_method);

        fread(b,1,1,fp);
        //ihdr_chunk.Interface_Method = b[0];
        printf("\npng Interface Method: %u ", ihdr_chunk.Interface_Method);        
        free(b);        
        set_Bytes_per_pixel();
        return 1;
    }
    else{
        return 0;
    }
}

struct TRNS_CHUNK trns_chunk;

void Get_Trns_Chunk(FILE* fp){
unsigned char* temp=malloc(4);
ssize_t fp_pos = ftell(fp);
int limit = 30000;
while(!feof(fp) && ftell(fp) < limit){
    fread(temp,1,1,fp);
    if(temp[0] == Trns_id[0]){
        ////printf("maybe found trns at %d",ftell(fp));
        fseek(fp,-1,1); //go back one to read the whole thing
        fread(temp,1,4,fp);
        if(compare_bytes_with_id(NULL,temp,Trns_id)){ // check if the 4 bytes match the identifier
            fseek(fp,-8,SEEK_CUR); // go back to the lengths
            fread(temp,1,4,fp); // read the length
            trns_chunk.length = *(unsigned int*)(temp);
            if(Little_Endian == 1)convert_big_to_little_endian(&trns_chunk.length); // get the chunk length
            ////printf("trns chunk len: %u ",trns_chunk.length);
            fseek(fp,4,SEEK_CUR);      
            break;
            }
    }
}
trns_chunk.Chunk_Data = (uint8_t*)malloc(trns_chunk.length * sizeof(uint8_t));
    for(int i = 0; i < trns_chunk.length; i++ ){
        fread(temp,1,1,fp);
        trns_chunk.Chunk_Data[i] = temp[0];
    }
    fseek(fp,fp_pos,SEEK_SET);
    free(temp);
}


int is_png(FILE* fp){
    
    unsigned char* b = malloc(8);
    fread(b,1,8,fp);
    for(int i=0;i<8;i++){
        if( b[i] != png_identifier[i] ){
        fclose(fp);
        free(b);
        return 0;
        } 
    }
    free(b);
    return 1;
}

void Print_Buffers(){            
}

void Convert_IDAT_BUF_TO_2D_PIX(){
    pixel_rows = (struct pixel**)malloc(ihdr_chunk.height*sizeof(struct pixel*));
    for(int i = 0 ; i < ihdr_chunk.height; i++ ){
        pixel_rows[i] = (struct pixel*)malloc(ihdr_chunk.width*sizeof(struct pixel));   
    }
    int ScanLineLength = (ihdr_chunk.width * bytes_pp);
    int BufferSize = ScanLineLength * ihdr_chunk.height;
    int row = 0;
    int col = 0;

    for(int i = 0; i<BufferSize;i+=ScanLineLength ){
     //   //////printf("\nScanline: %d",i);
        for(int j =bytes_pp +i; j < ScanLineLength+i+bytes_pp; j+=bytes_pp){
                    
            pixel_rows[row][col].r = 0;
            pixel_rows[row][col].g = 0;
            pixel_rows[row][col].b = 0;
            pixel_rows[row][col].A = 0;
           // pixel_rows[row][col].GR = 0;
                
                switch (ihdr_chunk.colour_type)
                {
                    case 0: 
                    pixel_rows[row][col].r = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].g = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].b = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].A = 255;

                    ;break;
                    case 3: 
                    pixel_rows[row][col].r = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].g = IDAT_Buffer[j-bytes_pp+1];
                    pixel_rows[row][col].b = IDAT_Buffer[j-bytes_pp+2];
                    pixel_rows[row][col].A = IDAT_Buffer[j-bytes_pp+3];
                    ;break;
                    case 4: 
                    pixel_rows[row][col].r = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].g = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].b = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].A = IDAT_Buffer[j-bytes_pp+1];
                    ;break;
                    case 6: 
                    pixel_rows[row][col].r = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].g = IDAT_Buffer[j-bytes_pp+1];
                    pixel_rows[row][col].b = IDAT_Buffer[j-bytes_pp+2];
                    pixel_rows[row][col].A = IDAT_Buffer[j-bytes_pp+3];
                    ;break;
                    case 2:
                    pixel_rows[row][col].r = IDAT_Buffer[j-bytes_pp];
                    pixel_rows[row][col].g = IDAT_Buffer[j-bytes_pp+1];
                    pixel_rows[row][col].b = IDAT_Buffer[j-bytes_pp+2];
		    pixel_rows[row][col].A = IDAT_Buffer[j-bytes_pp+3];
                    ;break;
                    default: 
                    ;break;
                }
                  //  //////printf("\n pixel_rows[%d][%d] r:%d g:%d b:%d A:%d GR:%d ",row,col,pixel_rows[row][col].r,pixel_rows[row][col].g,pixel_rows[row][col].b,pixel_rows[row][col].A,pixel_rows[row][col].GR);
                col++;
        }
        col = 0;
        row++;
    }
    //free(IDAT_Buffer);
    //free(IDAT_Buffer2);
}

int PNG_Init(char* FileName,int Option){
    FILE* fp;
    if(File_Exists(FileName)){
        PNG_file_size = get_file_size(FileName);
        fp = fopen(FileName,"rb");
        if(is_png(fp)){
            Little_Endian = is_little_endian();
            decode_IHDR(fp); 

            if(ihdr_chunk.colour_type == 3){ // 3 means that it is indexed
                bytes_pp = 4;
                get_PLTE(fp);
                Get_Trns_Chunk(fp);
                get_IDAT(fp);
                decode_IDAT();
                Convert_PLTE_To_Pixel();
                Make_IDATBUF_Plte();
                Optionsflag = Option;
                //Make_2D_pixel_struct_PLTE();
                if(Option == 2 || Option == 3){Convert_IDAT_BUF_TO_2D_PIX() ;}
                free(IDAT_Buffer2);
            }
            else{ // no indexes
                //decode without plte
                get_IDAT(fp);
                decode_IDAT();
                Undo_Filters();
                Optionsflag = Option;
                printf(" OPT: %d ",Optionsflag);
            	if(Option == 2 || Option == 3){Convert_IDAT_BUF_TO_2D_PIX();    }            
                free(IDAT_Buffer2);
        	}
        }
        else{
            perror("FILE IS NOT A PNG");
            return -2; 
        }
        fclose(fp);
        }
        else{   
            perror("FILE DOES NOT EXIST");
            return -1;
        }
        return 1;
        }

void PNG_Free_2dpixel(int height , pixel_pp* pp  ){
    for(int i = 0; i < ihdr_chunk.height; i++){
        free((*pp)[i]);
    }
    free( (*pp) );
}

//Deallocate all buffers
void PNG_Exit(){        
PNG_Free_2dpixel(ihdr_chunk.height,&pixel_rows);
free(IDAT_Buffer);
}

void PNG_Get_Pixelvals(uint8_t** plist){
    long long size = ihdr_chunk.width * ihdr_chunk.height * bytes_pp;
    (*plist) = malloc(size);
    for(int i = 0 ; i < size; i++){
        (*plist)[i] = IDAT_Buffer[i];
    }
    //memcpy(*plist,IDAT_Buffer,size);
}
//######## pngreading Functions #########//}



//........ PNGENCODING .........//{
uint8_t *def_IDAT_Buffer;
int bytespp=0;
int subpos=0;
uint8_t* scannd_scanline;


   /* Table of CRCs of all 8-bit messages. */
   unsigned long crc_table[256];
   
   /* Flag: has the table been computed? Initially false. */
   int crc_table_computed = 0;
   
   /* Make the table for a fast CRC. */
   void make_crc_table(void)
   {
     unsigned long c;
     int n, k;
   
     for (n = 0; n < 256; n++) {
       c = (unsigned long) n;
       for (k = 0; k < 8; k++) {
         if (c & 1)
           c = 0xedb88320L ^ (c >> 1);
         else
           c = c >> 1;
       }
       crc_table[n] = c;
     }
     crc_table_computed = 1;
   }
  

   /* Update a running CRC with the bytes buf[0..len-1]--the CRC
      should be initialized to all 1's, and the transmitted value
      is the 1's complement of the final running CRC (see the
      crc() routine below). */
   
   unsigned long update_crc(unsigned long crc, unsigned char *buf,
                            int len)
   {
     unsigned long c = crc;
     int n;
   
     if (!crc_table_computed)
       make_crc_table();
     for (n = 0; n < len; n++) {
       c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
     }
     return c;
   }
   
   /* Return the CRC of the bytes buf[0..len-1]. */
   unsigned long crc(unsigned char *buf, int len)
   {
     return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
   }

uint8_t* png_crc_buf;
//Writes the Chunk length and IDAT identifier
void write_chnk_len_IDAT(unsigned int chnk_len, FILE* fp){
    unsigned int Chunk_length = chnk_len;
    Chunk_length = __builtin_bswap32(Chunk_length);
    fwrite(&Chunk_length,1,4,fp);
    fwrite(IDAT_ID, 1, 4, fp);
}

void resize_image(int scale,uint8_t* input,struct IHDR_CHUNK* ihdr,int downscale){
  	int size = (ihdr->width*bytespp) * ihdr->height ;
  	uint8_t* scaled_idat = malloc(size);
  	ihdr->width = ihdr->width/scale;
  	ihdr->height = ihdr->height/scale;
	int scaled_idat_i=bytespp;
	int unscaled_idat_i=(bytespp)*scale;
	for(int i=0; i < ihdr->height; i++){
		for(int j=bytespp ; j < ihdr->width*bytespp; j+=bytespp){
			scaled_idat[scaled_idat_i-3] = input[unscaled_idat_i-3];
			scaled_idat[scaled_idat_i-2] = input[unscaled_idat_i-2];
			scaled_idat[scaled_idat_i-1] = input[unscaled_idat_i-1];
			scaled_idat[scaled_idat_i] = input[unscaled_idat_i];
		scaled_idat_i += bytespp;
		unscaled_idat_i +=bytespp*scale;
		}
	}
for(int i= 0; i < size; i++ ){
  input[i] = scaled_idat[i];
}
}

unsigned long Png_deflate(uint8_t *t, int ScanLineLen, int height, int bytepp,int blocksize, FILE *fp)
{
    unsigned char out[blocksize];
    int e = 0;
    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.opaque = 0;
    defstream.next_in = (Bytef* )t;
    defstream.avail_in = (height*ScanLineLen);
    deflateInit(&defstream, Z_DEFLATED);

    unsigned long bytes_written=0;
    do
    {
        int have; // is also chnk length
        defstream.avail_out = blocksize;
        defstream.next_out = out;
        e = deflate(&defstream, Z_FINISH);
        have = blocksize - defstream.avail_out;
       // printf("have: %u",have);
        write_chnk_len_IDAT(have,fp);
        bytes_written += fwrite(out, sizeof(char), have, fp);


        png_crc_buf = malloc(have+4);
        for(int i= 0; i < have; i++ ){
          png_crc_buf[i+4] = out[i]; 
        }
          png_crc_buf[0] = IDAT_ID[0];
          png_crc_buf[1] = IDAT_ID[1];
          png_crc_buf[2] = IDAT_ID[2];
          png_crc_buf[3] = IDAT_ID[3];
        unsigned long crc_val = crc(png_crc_buf,have+4);
        crc_val = __builtin_bswap32(crc_val);
        fwrite(&crc_val,1,4,fp);
        free(png_crc_buf);
    } while (defstream.avail_out == 0);
    deflateEnd(&defstream);
    printf("written %d",bytes_written);
    perror("zlib:");
    return bytes_written;
}

void Png_Encode(uint8_t *IDAT_input,  char *PngName, int width, int height, int bytepp, int option)
{
   // Exec_time_Start();
    bytespp = bytepp;
    struct IHDR_CHUNK ihdr;
    ihdr.height = height;
    ihdr.width = width;
    //resize_image(2,IDAT_input,&ihdr,1);
    unsigned long long ScanLineLen = (ihdr.width * bytespp) + 1;
    uint8_t *temp = malloc(ihdr.height * ScanLineLen);

    //write_image_to_console(IDAT_input,&ihdr);
    int index = 0;
    int IDAT_i = 0;

    for (unsigned long long i = 0; i < ihdr.height * ScanLineLen; i++)
    {
        if (i % ScanLineLen == 0 || i == 0)
        {
            temp[i] = 0;
        }
        else
        {
            temp[i] = IDAT_input[index];
            index++;
        }
    }
    uint8_t *ScanLine = malloc(ScanLineLen);
    free(ScanLine);
    FILE *fp = fopen(PngName, "wb");
    fwrite(png_identifier, 1, 8, fp);
    fwrite(standard_ihdr_length, 1, 4, fp);
    uint32_t w;
    uint32_t h;
    uint8_t b;
    b = 8;
    w = __builtin_bswap32(ihdr.width);
    h = __builtin_bswap32(ihdr.height);

    fwrite(IHDR_identifier, 1, 4, fp);
    fwrite(&w, 1, 4, fp);
    fwrite(&h, 1, 4, fp);

    uint8_t* buf = malloc(17);    

    buf[0] = IHDR_identifier[0];
    buf[1] = IHDR_identifier[1];
    buf[2] = IHDR_identifier[2];
    buf[3] = IHDR_identifier[3];

    buf[4] = (w & 0x000000ffUL);
    buf[5] = (w & 0x0000ff00UL) >> 8;
    buf[6] = (w & 0x00ff0000UL) >> 16;
    buf[7] = (w & 0xff000000UL) >> 24;

    buf[11] = (h & 0xff000000UL) >> 24;
    buf[10] = (h & 0x00ff0000UL) >> 16;
    buf[9] = (h & 0x0000ff00UL) >> 8;
    buf[8] = (h & 0x000000ffUL);

    buf[12] = 8;
    buf[13] = 6;
    buf[14] = 0;
    buf[15] = 0;
    buf[16] = 0;
    
    
    fputc(b, fp);
    b = 6;
    fputc(b, fp);
    b = 0;
    fputc(b, fp);
    fputc(b, fp);
    fputc(b, fp);

    unsigned long png_crc = 0;
    png_crc = crc(buf,17);
    png_crc = __builtin_bswap32( png_crc);
    fwrite(&png_crc,1,4,fp);
    free(buf);
    uint32_t chunk_len = 24;
    unsigned long e=0;
    unsigned long fp_p = ftell(fp)-8;
    //later on we need to split this up into multiple chunks
    chunk_len = Png_deflate(temp, ScanLineLen, ihdr.height, bytespp,16834, fp);
    fwrite(zeroes,1,4,fp);
    fwrite(IDAT_END_ID, 1, 4, fp);
   // free(buf);
    free(temp);
    free(IDAT_input);
    uint8_t EOI[] = {0xae, 0x42, 0x60, 0x82};

    fwrite(EOI, 1, 4, fp);
    // fwrite();
    fclose(fp);
   // Exec_time_stop();
}


        //........ PNGENCODING .........//]


        