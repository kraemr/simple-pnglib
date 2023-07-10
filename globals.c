#include "include/globals.h"


const unsigned char g_spng_IDAT_ID[4]={0x49,0x44,0x41,0x54};
const unsigned char g_spng_PLTE_ID[4]={0x50,0x4c,0x54,0x45};
const unsigned char g_spng_TRNS_ID[4]={0x74,0x52,0x4e,0x53};
const unsigned char g_spng_PNG_ID[8]={0x89,0x50,0x4e,0x47,0xd,0xa,0x1a,0xa};
const unsigned char g_spng_tExT_ID[]={116,69,88,116}; //tEXT
const unsigned char g_spng_zTXt_ID[]={122,84,88,116}; //zTXt
const unsigned char g_spng_iTXt_ID[]={105,84,88,116}; //zTXt
const unsigned char g_spng_bkgd_ID[]={98,75,71,68}; //zTXt
const unsigned char g_spng_cHRM_ID[]={99,72,82,77}; //zTXt
const unsigned char g_spng_gAMA_ID[]={103,65,77,65}; //zTXt
const unsigned char g_spng_iCCP_ID[]={105,67,67,80}; //zTXt
const unsigned char g_spng_sRGB_ID[]={115,82,71,66}; //zTXt
const unsigned char g_spng_sBIT_ID[]={115,66,73,84}; //zTXt
const unsigned char g_spng_sPLT_ID[]={115,80,76,84}; //zTXt
const unsigned char g_spng_tIME_ID[]={116,73,77,69}; //zTXt
const unsigned char g_spng_hIST_ID[]={104,73,83,84}; //zTXt
const unsigned char g_spng_IHDR_ID[]={0x49,0x48,0x44,0x52};// IHDR
const unsigned char g_spng_phys_ID[]={112,72,89,115}; //pHYs

unsigned char g_spng_iccp_allocated;
unsigned int g_spng_crc; // global crc val
unsigned int g_spng_bytes_rwritten;
unsigned char g_spng_is_little_endian;
unsigned char g_spng_bkgd[6];

unsigned int g_spng_is_allocated; //keeps track if it has been Initialized by the same program
unsigned int g_spng_plte_is_allocated; //keeps track if plte is initialized
unsigned int g_trns_len;
unsigned int g_spng_has_trns;

struct SPNG_AUTHORINFO g_spng_author_info;
struct SPNG_INFO g_spng_spnginf; //keeps track of the currently loaded images dimensions,bytespp,clrtype ...
struct SPNG_PIXEL* g_spng_plte_pixels;
unsigned int g_spng_plte_len;





void spng_change_endian(unsigned int * n){
*n = (*n >> 24) | ((*n >> 8) & 0x0000ff00) | ((*n<<8) & 0x00ff0000) | (*n << 24);
}

#ifdef SPNGLIB_DEBUG_BENCHMARK
    float SPNG_BENCH_START;
    float SPNG_BENCH_END;
    float SPNG_BENCH_ELAPSED;

    unsigned int SUB_COUNT = 0;
    unsigned int UP_COUNT = 0;
    unsigned int AVG_COUNT = 0;
    unsigned int PAETH_COUNT = 0;
    unsigned int NOFILTER_COUNT = 0;

    
    void spng_bench_start(){
       SPNG_BENCH_START = (float)clock()/CLOCKS_PER_SEC;
    }

     void spng_bench_end(char * endmsg){
        SPNG_BENCH_END = (float)clock()/CLOCKS_PER_SEC;
        SPNG_BENCH_ELAPSED = SPNG_BENCH_END - SPNG_BENCH_START;
        printf("%s:%f\n",endmsg,SPNG_BENCH_ELAPSED);
    }


    void dump_buffer_to_file(const char* buffer, size_t buffer_size, const char* file_name) {
    // Open the file for writing in binary mode
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        printf("Error: failed to open file %s for writing\n", file_name);
        return;
    }

    // Write the contents of the buffer to the file as hex
    for (size_t i = 0; i < buffer_size; i++) {
        fprintf(file, "%02x ", (unsigned char)buffer[i]);
    }

    // Close the file
    fclose(file);
    }
    
#endif