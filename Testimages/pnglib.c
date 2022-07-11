#include "pnglib.h"
//******** Variable Definitions ********//{
struct pixel** pixel_rows; //TODO: Remove this later out of .h
unsigned int Little_Endian; // if 1 System is little endian else Big endian
int bytes_pp=0;
int enc_bytespp = 0; // THIS IS USED FOR THE ENCODER FUNCTIONS

ssize_t  PNG_file_size=0; // FILE SIZE IN BYTES
uint8_t* IDAT_Buffer; //this first holds the compressed bytes afterwards filtered and uncompressed bytes
uint8_t* IDAT_Buffer2; //for copying the IDAT_buffer and operating on it
uint8_t* def_IDAT_Buffer;
uint8_t* png_crc_buf;
//uint8_t* scannd_scanline;
uint8_t* ScanLine;
uint8_t* previous_ScanLine;
int ScanLineLength=0;
int crc_table_computed = 0;
int bytespp=0;
int subpos=0;
int is_initialized = 0;
/* Table of CRCs of all 8-bit messages. */

unsigned long crc_table[256];
unsigned long IDAT_Length=0;
/* Flag: has the table been computed? Initially false. */

struct IHDR_CHUNK ihdr_chunk;
struct PLTE_CHUNK plte_chunk;
struct TRNS_CHUNK trns_chunk;
struct z_stream_s zst;
struct pixel** pixel_rows;

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
Because this function will write width and height to these vars*/
void PNG_Get_Dim(int* w, int* h){
    *w = ihdr_chunk.width;
    *h = ihdr_chunk.height;
}


void Print_Pixel(struct pixel px){
    printf("Pixel Values r: %u, b: %u, g: %u, A: %u ",px.r,px.g,px.b,px.A);
}

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

long long int PNG_get_size(char *pathname){
    struct stat file_stat;
    stat(pathname,&file_stat);
    return file_stat.st_size;
}

int PNG_file_exists(char* file_name){
  if( access( file_name, F_OK ) != -1)return 1;
  else return 0;  
    //perror("file is not found");
}
int isLittle_Endian(){
    int num = 1;
    if (*(char *)&num == 1)return 1;//printf("Little-Endian\n");
    else return 0;//printf("Big-Endian\n");
}


void PNG_change_endian(unsigned int* n){
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



unsigned int read_chunk_length(FILE* fp){
    unsigned char* temp=malloc(4);
    fseek(fp,-8,SEEK_CUR); // seek to the start of the lengths
    fread(temp,1,4,fp);
    unsigned int length=*(unsigned int*)(temp);

    if(Little_Endian){
        fseek(fp,4,SEEK_CUR);
        free(temp);
        PNG_change_endian(&length);
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
    //print_decoded();
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
                if(Little_Endian == 1)PNG_change_endian(&plte_chunk.length); // get the chunk length
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
    if(Little_Endian == 1)PNG_change_endian(&plte_chunk.CRC);
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

int set_Bytes_per_pixel(int clr,int bit_depth){
    int bpp = 0;
        switch (clr)
        {
        case Greyscale: bpp = (1 * bit_depth) / 8 ;break;
        case Greyscale_Alpha: bpp = (2 * bit_depth) / 8; break;
        case Truecolour_Alpha: bpp = (4 * bit_depth) / 8; break;
        case Truecolour: bpp = (3 * bit_depth) / 8; break;
        case Indexed_colour: bpp = (4 * bit_depth) / 8; break;        
        default:
            break;
        }
    return bpp;
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
 //  printf(" IDATBUF len: %u h: %u w: %u bpp: %u ",IDAT_i,ihdr_chunk.height,ihdr_chunk.width,bytes_pp);
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
        Little_Endian & 1 ?PNG_change_endian(&ihdr_chunk.width):0;
        printf("\npng widht: %u ", ihdr_chunk.width);

        fread(b,1,4,fp);
        ihdr_chunk.height = *(unsigned int*)(b);
        Little_Endian & 1 ?PNG_change_endian(&ihdr_chunk.height):0;
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
        bytes_pp = set_Bytes_per_pixel(ihdr_chunk.colour_type,ihdr_chunk.bit_depth);
        printf("bytes_pp: %d",bytes_pp);
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
            if(Little_Endian == 1)PNG_change_endian(&trns_chunk.length); // get the chunk length
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
void PNG_Get_PNGINFO(PNG_INFO* pnginf_ref){
    (*pnginf_ref) = ihdr_chunk;
}
//Pass your 2d pixel struct unitialized as a variable and just pass your buffer here
// expects RGB or RGBA as input
void PNG_Get_Pixelpp(pixel_pp* ppp,uint8_t* in,uint8_t clr){
    printf("\n Converting IDAT to 2d PIX\n");
    (*ppp) = (struct pixel**)malloc(ihdr_chunk.height*sizeof(struct pixel*));
    for(int i = 0 ; i < ihdr_chunk.height; i++ ){
        (*ppp)[i] = (struct pixel*)malloc(ihdr_chunk.width*sizeof(struct pixel));   
    }
    int ScanLineLength = (ihdr_chunk.width * bytes_pp);
    int BufferSize = ScanLineLength * ihdr_chunk.height;
    int row = 0;
    int col = 0;

    for(int i = 0; i<BufferSize;i+=ScanLineLength ){
        for(int j =bytes_pp +i; j < ScanLineLength+i+bytes_pp; j+=bytes_pp){
                    
            (*ppp)[row][col].r = 0;
            (*ppp)[row][col].g = 0;
            (*ppp)[row][col].b = 0;
            (*ppp)[row][col].A = 0;                
                switch (clr)
                {
                    case Greyscale: 
                    (*ppp)[row][col].r = in[j-bytes_pp];
                    (*ppp)[row][col].g = in[j-bytes_pp];
                    (*ppp)[row][col].b = in[j-bytes_pp];
                    (*ppp)[row][col].A = 255;

                    ;break;
                    case Indexed_colour: 
                    (*ppp)[row][col].r = in[j-bytes_pp];
                    (*ppp)[row][col].g = in[j-bytes_pp+1];
                    (*ppp)[row][col].b = in[j-bytes_pp+2];
                    (*ppp)[row][col].A = in[j-bytes_pp+3];
                    ;break;
                    case Greyscale_Alpha: 
                    (*ppp)[row][col].r = in[j-bytes_pp];
                    (*ppp)[row][col].g = in[j-bytes_pp];
                    (*ppp)[row][col].b = in[j-bytes_pp];
                    (*ppp)[row][col].A = in[j-bytes_pp+1];
                    ;break;
                    case Truecolour_Alpha: 
                    (*ppp)[row][col].r = in[j-bytes_pp];
                    (*ppp)[row][col].g = in[j-bytes_pp+1];
                    (*ppp)[row][col].b = in[j-bytes_pp+2];
                    (*ppp)[row][col].A = in[j-bytes_pp+3];
                    ;break;
                    case Truecolour:
                    (*ppp)[row][col].r = in[j-bytes_pp];
                    (*ppp)[row][col].g = in[j-bytes_pp+1];
                    (*ppp)[row][col].b = in[j-bytes_pp+2];
		            (*ppp)[row][col].A = in[j-bytes_pp+3];
                    ;break;
                    default: ;break;
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

int PNG_Init(char* FileName){
    FILE* fp;
    if(PNG_file_exists(FileName)){
        PNG_file_size = PNG_get_size(FileName);
        fp = fopen(FileName,"rb");
        if(is_png(fp)){
            Little_Endian = isLittle_Endian();
            decode_IHDR(fp); 

            if(ihdr_chunk.colour_type == 3){ // 3 means that it is indexed
                bytes_pp = 4;
                get_PLTE(fp); // PLTE is always after ihdr and before IDAT 
                Get_Trns_Chunk(fp); // Trns is after plte and before idat 
                get_IDAT(fp);
                decode_IDAT();
                Convert_PLTE_To_Pixel(); // converts the plte entries to pixels
                Make_IDATBUF_Plte(); //
                free(IDAT_Buffer2);
                is_initialized = 1;
            }
            else{ 
                //decode without plte
                get_IDAT(fp);
                decode_IDAT();
                Undo_Filters();
                printf(" OPT: %d ",Optionsflag);
                free(IDAT_Buffer2);
                is_initialized = 1;
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
  //  PNG_Free_2dpixel(ihdr_chunk.height,&pixel_rows);
    free(IDAT_Buffer);
}

// IDAT buffer is not in RGBA so it hast to be converted
void PNG_Get_Pixelvals_RGBA(uint8_t** plist){


    long ScanLen = ihdr_chunk.width * bytes_pp;
    long size = ihdr_chunk.height * ScanLen;
    struct pixel px;
    (*plist) = malloc(ihdr_chunk.width*ihdr_chunk.height*4);
    if(ihdr_chunk.colour_type == Truecolour_Alpha){
        perror("Pixelvals are already RGBA");
    }
    int i = bytes_pp-1;
    if(ihdr_chunk.colour_type == Greyscale){
        i = bytes_pp;
    }
    int j = 0 ;
    if(ihdr_chunk.colour_type == Truecolour_Alpha || ihdr_chunk.colour_type == Truecolour ) j = 3;
    while(i < size){
        
        switch (ihdr_chunk.colour_type)
        {
            case Greyscale:
            printf("Greyscale");
            (*plist)[j] = IDAT_Buffer[i];
            j++;
            (*plist)[j] = IDAT_Buffer[i];
            j++;
            (*plist)[j] = IDAT_Buffer[i];
            j++;
            (*plist)[j] = 255;
            j++;
            ;break;
            case Greyscale_Alpha:
            (*plist)[j] = IDAT_Buffer[i-1];
            j++;
            (*plist)[j] = IDAT_Buffer[i-1];
            j++;
            (*plist)[j] = IDAT_Buffer[i-1];
            j++;
            (*plist)[j] = IDAT_Buffer[i];
            j++;
            break;
            case Truecolour:
            (*plist)[j-3] = IDAT_Buffer[i-2];
            (*plist)[j-2] = IDAT_Buffer[i-1];
            (*plist)[j-1] = IDAT_Buffer[i];
            (*plist)[j] = 255;
            j+=4;
            break;
            case Truecolour_Alpha:
            (*plist)[j-3] = IDAT_Buffer[i-3];
            (*plist)[j-2] = IDAT_Buffer[i-2];
            (*plist)[j-1] = IDAT_Buffer[i-1];
            (*plist)[j] = IDAT_Buffer[i];
            j+=bytes_pp;
            ;break;
        }
        
        if(ihdr_chunk.colour_type != Indexed_colour)i+=bytes_pp;
        else i++;
    }
  
}

void PNG_Get_Pixelvals(uint8_t** plist){
    int size = ihdr_chunk.width * ihdr_chunk.height * bytes_pp;
    (*plist) = malloc(size);
    for(int i = 0; i < size;i++){
        (*plist)[i] = IDAT_Buffer[i];
    }
}

//######## pngreading Functions #########//}

//........ PNGENCODING .........//{
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
   unsigned long PNG_update_crc(unsigned long crc, unsigned char *buf,int len)
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
     return PNG_update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
    }
//Writes the Chunk length and IDAT identifier
void write_chnk_len_IDAT(unsigned int chnk_len, FILE* fp){
    unsigned int Chunk_length = chnk_len;
    Chunk_length = __builtin_bswap32(Chunk_length);
    fwrite(&Chunk_length,1,4,fp);
    fwrite(IDAT_ID, 1, 4, fp);
}

void PNG_mipmap(uint8_t** buf,uint8_t** scaled_out,int h, int w,int scale,int clr){
    pixel_pp pp;
    pixel_pp pp_sc;//scaled 2d pixel
   // PNG_make2d_Idat(&p_vals2d,buf,w,h);
    PNG_Get_Pixelpp(&pp,buf,clr); //this throws a warning but it works ?? TODO: test if it causes problems
    w/=scale;
    h/=scale;

    (*scaled_out )= malloc(w*h*4);
    (pp_sc)= (struct pixel**)malloc(h*sizeof(struct pixel*));
    int idat_i=3;
    for(int i= 0; i < h; i++){
        (pp_sc)[i] = (struct pixel*)malloc(w * sizeof(struct pixel) );
        for(int j = 0; j < w  ; j++){
        pp_sc[i][j].r = 
        (pp[i*scale][j*scale].r + pp[i*scale][j*scale+1].r 
        + pp[i*scale+1][j*scale].r + pp[i*scale+1][j*scale+1].r)/4;
          
        pp_sc[i][j].g = 
        (pp[i*scale][j*scale].g + pp[i*scale][j*scale+1].g 
        + pp[i*scale+1][j*scale].g + pp[i*scale+1][j*scale+1].g)/4;
          
         pp_sc[i][j].b = 
        (pp[i*scale][j*scale].b + pp[i*scale][j*scale+1].b 
        + pp[i*scale+1][j*scale].b + pp[i*scale+1][j*scale+1].b)/4;
          
        pp_sc[i][j].A = 
        (pp[i*scale][j*scale].A + pp[i*scale][j*scale+1].A 
        + pp[i*scale+1][j*scale].A + pp[i*scale+1][j*scale+1].A)/4;
            
        (*scaled_out)[idat_i-3] = pp_sc[i][j].r;
        (*scaled_out)[idat_i-2] = pp_sc[i][j].g;
        (*scaled_out)[idat_i-1] = pp_sc[i][j].b;
        (*scaled_out)[idat_i] = pp_sc[i][j].A;
        idat_i+=4;
        }
        free(pp_sc[i]);
    }
    free(pp_sc);
    PNG_Free_2dpixel((h*scale),&pp);
}

int ApplyFilterScan(){

}

unsigned long PNG_deflate(uint8_t *t, int ScanLineLen, int height, int bytepp,int blocksize,int bits, FILE *fp)
{
    unsigned char out[blocksize];
    int e = 0;
    z_stream defstream;
    defstream.zalloc = 0;
    defstream.zfree = 0;
    defstream.opaque = 0;
    defstream.next_in = (Bytef* )t;
   switch(bits)
   {
   case 4:
                  defstream.avail_in = (height*ScanLineLen)/2;
    break;
    case 2:
               defstream.avail_in = (height*ScanLineLen)/4;
    break;
    case 1:
           defstream.avail_in = (height*(ScanLineLen -1 ) )/8;
           defstream.avail_in += height; 
    break;
   
   default:
       defstream.avail_in = (height*ScanLineLen);
    break;
   }
   printf("\navail in %d \n",defstream.avail_in);
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

int PNG_calc_filterval(int ScanLineLen,int bytepp){
    ScanLine[0] = 0;
    int filter=UP;
    /*
    uint8_t* curblock = malloc(bytepp);
    uint8_t* lastblock = malloc(bytepp);
    lastblock[0] = 0;
    lastblock[1] = lastblock[0];
    lastblock[2] = lastblock[0];
    lastblock[3] = lastblock[0];
    */
    //check if the ScanLine is the same as the One Above
    for(int i = 1; i < ScanLineLen; i++){
        if(ScanLine[i] != previous_ScanLine[i]){
            filter = -2; 
        }
       // filter = -2;
    }
    if(filter == 2){
        //UP is NOT possible
        //check if sub is possible
        //return UP;
        //printf(" UP encoding");
        return UP;
    }
    if(filter == -2){
        //NOFILTER
       // printf("NO Filter");
        return NOFILTER;
    }
}

// transforms the input Scanline according to the filter and the given arguments
int applyFilter(uint8_t* fil_Scan,int len){
    int filter = ScanLine[0];
    switch (filter)
    {
    case UP:
        fil_Scan[0] = UP;
        for(int i = 1; i < len ; i++ ){
           // printf(" %u prev: %d",fil_Scan[i],previous_ScanLine[i]);
            fil_Scan[i] = ScanLine[i] - previous_ScanLine[i];
        }
    break;
    case NOFILTER:
        for(int i = 0; i < len; i++){
            fil_Scan[i] = ScanLine[i];
        }
    break;
    //gets the last pixel and subs the corresponding values unless it is under 8bits
    // if it is under 8 then 
    case SUB:
        //uint8_t* last;
        //uint8_t* current;
        for(int i  = enc_bytespp ;i < len; i += enc_bytespp){
            //get_last_block();
            //get last 
            //get current 
            //last(x )
        }
    break;
    default:
        break;
    }
}

// this copies the Scanline buffer to the filtered buffer, which gets written to file
int copyScantoBuf(uint8_t* fil_buf,uint8_t* fil_Scan,int len,int buf_i){
    for(int i = 0; i < len; i++){
        fil_buf[buf_i] = fil_Scan[i];
        buf_i++;
    }
    return buf_i;
}
//expects RGB or RGBA
void RGB_to_GreyScale(uint8_t* in,uint8_t** out,int w,int h,int isRGB,int gr_hasAlpha,int bitdepth){
    int size = 0;
    int bytepp = 0;
    int grbytepp = 0;
    uint8_t val=0;
    uint8_t bit_pos=0;
    uint8_t OrMask=0;
    if(gr_hasAlpha) grbytepp = 2;
    else grbytepp = 1;
    if(isRGB){
        bytepp = 3;
        size = ((w * bytepp)) * h; 
    }
    else{
        bytepp = 4;
        size = ((w * bytepp )) * h; 
    }
    int new_size = 0;
    new_size = (w * grbytepp)* h ;
    (*out) = malloc(new_size);
    struct pixel pix;
    pix.r = 0;
    pix.b = 0; 
    pix.g = 0;
    pix.A = 255;
    int out_i=0;
    /*
    float Y = 0;
    float R = 0;
    float G = 0;
    float B = 0;
    */
   // Y = 0.299 R + 0.587 G + 0.114 B
    //printf("size: %d",size);

    if(bitdepth == 8){
        for(int i = bytepp; i < size; i+=bytepp){
            if(bytepp == 4){
                pix.r = in[i-3];
                pix.g = in[i-2];
                pix.b = in[i-1];
                pix.A = in[i];
/*
            R = (float)pix.r ;
            R *= 0.299f;
            G = (float)pix.g;
            G  *= 0.587f;
            B = (float)pix.b;
            B *= 0.114f;
*/
                (*out)[out_i] = (pix.r + pix.b + pix.g) /3;
                    if(gr_hasAlpha){
                        out_i++;
                        (*out)[out_i] = pix.A;
                }
            }
            else{
            pix.r = in[i-2];
            pix.g = in[i-1];
            pix.b = in[i];
            (*out)[out_i] = (pix.r + pix.b + pix.g) / 3;
            if(gr_hasAlpha) {
                out_i++;
                (*out)[out_i] = pix.A;
            }

            }
        out_i++;
        }
    }
    else{
        for(int i =bytepp-1 ; i < size;i+=bytepp){
            if(isRGB){
                pix.r = in[i-2];
                pix.g = in[i-1];
                pix.b = in[i];      
            }else{
                pix.r = in[i-3];
                pix.g = in[i-2];
                pix.b = in[i-1];      
            }
            uint8_t tmp=0;
            switch (bitdepth)
            {
            case 1:
                tmp = (pix.r+pix.b+pix.g)/3;
                tmp > 128 ? (val = (1 << bit_pos)  | val) : 0;
              //  printf(" %d", i);
                bit_pos++;             
                if(bit_pos == 8 || bit_pos == w*h) {
                    bit_pos = 0;
                    //(*out)[out_i] = (val * 0x0202020202ULL & 0x010884422010ULL) % 1023; //reverses the bits with 64bit multiplication
                    (*out)[out_i] =((val * 0x0802LU & 0x22110LU) | (val * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
                    val =0;
                    out_i++;
                }
           ; break;
            case 2:

            break;
            case 4:

            break;
            
            default:
                break;
            }
        }
    }
    //printf(" %d ",out_i);

    //printf("\n out_i: %d",out_i);
}

//this creates the filtered buffer to write to the file
void makefilteredbuffer(uint8_t* in, uint8_t* temp, int bufsize, int len){
    ScanLine = malloc(len);
    uint8_t* filtered_Scan=malloc(len);
    previous_ScanLine = malloc(len);
    int temp_i=0;
    int in_i=0;
    uint8_t first = 0;
    int i = 0;
    //we need this because in some cases there are average filters or up filters even if it is the first scanline
    //in cases like these you are suppossed to assume the values are all 0
    for(int j = 0; j < len; j++){
        previous_ScanLine[j] = 0; 
    }    
    while(i < bufsize ){
        if(!first){
            for(int j = 0; j < len; j++){
                previous_ScanLine[j] = ScanLine[j];
            }
        }
        for(int j = 1; j < len; j++){
            ScanLine[j] = in[in_i];
            in_i++;
        }
        first = 0;
        ScanLine[0] = 0;
        //ScanLine[0] = PNG_calc_filterval(len,0);
        applyFilter(filtered_Scan,len);
        temp_i = copyScantoBuf(temp,filtered_Scan,len,temp_i);
        i+=len;
    }
    free(ScanLine);
    free(filtered_Scan);
    free(previous_ScanLine);
    free(in);
}
//counts unique values and checks if unique values < 256
// if < 256 -> indexed colour is possible 
int indexed_ccheck(uint8_t* in, int len){
    int unique_count=0;
    uint8_t check=0;
    for(int i = 4; i < len ; i+=4){

        for(int j = 4; j < len ; j+=4){
             
            if( in[i-3] == in[j-3] &&
                in[i-2] == in[j-2] &&
                in[i-1] == in[j-1])
            {
            check = 0;
            break;
          }
            else{
                check = 1;
            }
        }
        check & 1 ? unique_count++:0;
    }
    printf("unique vals counted: %d",unique_count);
    return unique_count < 256 ? 1 : 0; // will return 1 if unique_count is true
}



struct plte_map_element{
    struct pixel px; // input
    int value;
};
int plte_map_size = 0;
struct plte_map_element* pmap;

void init_map(int n){
    pmap = malloc(n * sizeof(struct plte_map_element));
    plte_map_size = n;
    for(int i = 0; i < n; i++){
        pmap[i].value = -1;
    }
}

int get_map_val(struct pixel p){
    for(int i = 0; i < plte_map_size; i++ ){
        if(p.r != pmap[i].px.r) continue;;
        if(p.b != pmap[i].px.b) continue;
        if(p.g != pmap[i].px.g) continue;
        if(p.A != pmap[i].px.A) continue;
        return pmap[i].value;
    }
    return -2; // Value does not exist
}

void set_map_val(struct pixel p,int value){
    if(value < 0){
        return;
    }
    pmap[value].px.r = p.r;
    pmap[value].px.g = p.g;
    pmap[value].px.b = p.b;
    pmap[value].px.A = p.A;
    pmap[value].value = value;
}

// Pass a plte chunk struct aand trns as reference if isRGB = 0 clr is RGB no Alpha else RGBa so write trns 
int make_pixel_map(uint8_t* in,uint8_t** indexes,int bufsize,int isRGB){
    int plte_i = 0;
    int index_i =0;
    int bytepp = 0;
    if(isRGB) bytepp = 3;
    else bytepp = 4;
    init_map(bufsize);
    printf("bufsize: %d",bufsize);
    struct pixel px;
    int val = 0;
    int in_i = bytepp-1;
    for(int i = bytepp-1; i < bufsize;i+=bytepp ){
        if(index_i % ScanLineLength == 0){
            (*indexes)[index_i] = 0;
            printf("");
            index_i++;
            continue;
        }
        px.r = in[in_i-3];
        px.g = in[in_i-2];
        px.b = in[in_i-1];
        px.A = in[in_i];
        val = get_map_val(px);

        if(val == -2){
            //value not mapped 
            set_map_val(px,plte_i);
            (*indexes)[index_i] = get_map_val(px); 
            //printf("\nno val for key %u %u %u %u",px.r,px.g,px.b,px.A);
            index_i++;
            in_i+=bytepp;
            plte_i++;
        }
        else if(val > -1){
           //value is mapped
           //printf(" %d",val);
           (*indexes)[index_i] = val;
            index_i++;
            in_i+=bytepp;
            continue; 
        }
    }
    return plte_i;
}

//Expects RGBA for now
void Png_Encode(uint8_t *IDAT_input,  char *PngName, int width, int height, int clr_out,int clr_in,unsigned char bitdepth)
{
    Little_Endian = isLittle_Endian();
    int indexed_possible = 0;
    struct IHDR_CHUNK ihdr;
    ihdr.height = height;
    ihdr.width = width; 
    ihdr.colour_type = clr_out;
    ihdr.bit_depth = bitdepth;
    enc_bytespp = set_Bytes_per_pixel(clr_in,ihdr.bit_depth);
    uint8_t* in_buf = NULL; 
    uint8_t* buf = NULL;  
    uint8_t* pltebuf = NULL;
    uint8_t* pltebuf2 = NULL;
    uint32_t plte_size = 0;
    unsigned long png_crc = 0;

    //this will contain the transformed pixel values in a 1D list 
    //enc_bytespp will define how to get a pixels values
    // printf("clr in: %d  clr out: %d bytespp: %d ",clr_in,clr_out,enc_bytespp);
    //TODO: relocate the if statements into another function for more readability
    if(clr_in == clr_out){
        enc_bytespp = set_Bytes_per_pixel(clr_out,8);
        in_buf = malloc(ihdr.height*ihdr.width*enc_bytespp);
        for(int i = 0; i < ihdr.height*ihdr.width*enc_bytespp;i++){
            in_buf[i] = IDAT_input[i];
        }
        //RGB_to_GreyScale(IDAT_input,&in_buf,ihdr.width,ihdr.height,0,0);
    }
    else if(clr_out == Truecolour && clr_in == Truecolour_Alpha){
        enc_bytespp = set_Bytes_per_pixel(clr_out,8);
        in_buf = malloc(ihdr.height*ihdr.width*enc_bytespp);
        int in_buf_i = 2;
        // 
        for(int i = 3; i < ihdr.height*ihdr.width*4;i+=4){
            in_buf[in_buf_i-2] = IDAT_input[i-3];
            in_buf[in_buf_i-1] = IDAT_input[i-2];
            in_buf[in_buf_i] = IDAT_input[i-1];
            in_buf_i+=enc_bytespp;         
        }
    }
    else if(clr_in == Truecolour_Alpha && clr_out == Greyscale){
        enc_bytespp = set_Bytes_per_pixel(clr_out,8);
        RGB_to_GreyScale(IDAT_input,&in_buf,ihdr.width,ihdr.height,0,0,bitdepth);
    }
    else if(clr_in == Truecolour_Alpha && clr_out == Greyscale_Alpha){
        enc_bytespp = set_Bytes_per_pixel(clr_out,8);
        RGB_to_GreyScale(IDAT_input,&in_buf,ihdr.width,ihdr.height,0,1,bitdepth);
    }
    else if(clr_in == Truecolour_Alpha && clr_out == Indexed_colour){
        //NO FILTERING IS NEEEDED
        indexed_possible = indexed_ccheck(IDAT_input,ihdr.width * ihdr.height);
        enc_bytespp = 1;    
    }
    printf("enc bpp %d",enc_bytespp);
    unsigned long ScanLineLen = (ihdr.width * enc_bytespp) + 1;
    int Bufsize = ScanLineLen*ihdr.height;
    printf(" %d ",Bufsize);
    uint8_t* temp = malloc(Bufsize);
    
    printf("indexed is possible %d",indexed_possible);
    if(indexed_possible == 0 && bitdepth >=8)makefilteredbuffer(in_buf,temp,Bufsize,ScanLineLen);
    else if(bitdepth < 8){
        int temp_i = 0;
        for(int i = 0; i < Bufsize;i++){
            if(i % ScanLineLen == 0){
                temp[i] = 0; 
            }
            else{
                 temp[i] = in_buf[temp_i];
                 temp_i++; 
            }
        }
        free(in_buf);
    }
    else{
        ScanLineLength = ScanLineLen;
        Bufsize = ihdr.height *(  (ihdr.width * 4) +1 );
        printf("scanlinelength: %d",ScanLineLength);
        uint32_t elements = make_pixel_map(IDAT_input,&temp,Bufsize+ihdr.width*3,0); //TODO: IMPLEMENT THIS
        plte_size = 8+elements*3;
        pltebuf = malloc(plte_size);
        pltebuf2 = malloc(plte_size-4);

        pltebuf[0] = ( (plte_size-8) & 0xff000000UL) >> 24;
        pltebuf[1] = ( (plte_size-8)  & 0x00ff0000UL) >> 16;
        pltebuf[2] = ( (plte_size-8)  & 0x0000ff00UL) >> 8;
        pltebuf[3] = ( (plte_size-8)  & 0x000000ffUL) ;
        pltebuf[4] = PLTE_identifier[0];
        pltebuf[5] = PLTE_identifier[1];
        pltebuf[6] = PLTE_identifier[2];
        pltebuf[7] = PLTE_identifier[3];
        pltebuf2[0] = PLTE_identifier[0];
        pltebuf2[1] = PLTE_identifier[1];
        pltebuf2[2] = PLTE_identifier[2];
        pltebuf2[3] = PLTE_identifier[3];

        uint32_t plte_i = 0;
        uint32_t plte_i2 = 6;

        for(int i = 10; i < ( (elements) * 3 )+8;i+=3 ){
            pltebuf[i-2] = pmap[plte_i].px.r;
            pltebuf[i-1] = pmap[plte_i].px.g;
            pltebuf[i] = pmap[plte_i].px.b;
            pltebuf2[plte_i2-2] =  pmap[plte_i].px.r;
            pltebuf2[plte_i2-1] =  pmap[plte_i].px.g;
            pltebuf2[plte_i2] =  pmap[plte_i].px.b;
            plte_i2+=3;
            printf(" %d ",i);
            plte_i++;
        }

    }
    //free(in_buf);
    FILE *fp = fopen(PngName, "wb");
    fwrite(png_identifier, 1, 8, fp);
    fwrite(standard_ihdr_length, 1, 4, fp);
    uint32_t w = 0;
    uint32_t h = 0;
    uint8_t b = 0; 
    w = ihdr.width;
    h = ihdr.height;
    // here we convert to big endian if system is little endian else do nothing and just write the values
    if(Little_Endian){
        PNG_change_endian(&w);
        PNG_change_endian(&h);
    }
    fwrite(IHDR_identifier, 1, 4, fp);
    fwrite(&w, 1, 4, fp);
    fwrite(&h, 1, 4, fp);
    buf = malloc(17);
    buf[0] = IHDR_identifier[0];
    buf[1] = IHDR_identifier[1];
    buf[2] = IHDR_identifier[2];
    buf[3] = IHDR_identifier[3];
    // gets the individual bytes out of the 4 bytes value
    buf[4] = (w & 0x000000ffUL);
    buf[5] = (w & 0x0000ff00UL) >> 8;
    buf[6] = (w & 0x00ff0000UL) >> 16;
    buf[7] = (w & 0xff000000UL) >> 24;
    buf[11] = (h & 0xff000000UL) >> 24;
    buf[10] = (h & 0x00ff0000UL) >> 16;
    buf[9] = (h & 0x0000ff00UL) >> 8;
    buf[8] = (h & 0x000000ffUL);
    //color type and bit depth 
    buf[12] = ihdr.bit_depth;
    buf[13] = ihdr.colour_type;
    buf[14] = 0;
    buf[15] = buf[14];
    buf[16] = buf[15];
    b = ihdr.bit_depth;
    fputc(b, fp);
    b = ihdr.colour_type;
    fputc(b, fp);
    b = 0;
    fputc(b, fp);
    fputc(b, fp);
    fputc(b, fp); 
    png_crc = crc(buf,17);
    png_crc = __builtin_bswap32( png_crc);
    fwrite(&png_crc,1,4,fp);
    free(buf); // writing ihdr is done 

    if(clr_out == Indexed_colour && indexed_possible == 1){
        
        fwrite(pltebuf,1,plte_size,fp);
        png_crc = crc(pltebuf2,plte_size-4);
        png_crc = __builtin_bswap32( png_crc);
        free(pltebuf);
        fwrite(&png_crc,1,4,fp);
                free(pltebuf2);

    }
 
    unsigned long e=0;
    unsigned long fp_p = ftell(fp)-8;
    e = PNG_deflate(temp, ScanLineLen, ihdr.height, enc_bytespp,16384,bitdepth ,fp);
    fwrite(zeroes,1,4,fp);
    fwrite(IDAT_END_ID, 1, 4, fp);    
    free(temp);
    uint8_t EOI[] = {0xae, 0x42, 0x60, 0x82};
    fwrite(EOI, 1, 4, fp);
    fclose(fp);
    
   // Exec_time_stop();   
}
//........ PNGENCODING .........//]


        
