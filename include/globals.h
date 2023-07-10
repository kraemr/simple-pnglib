#include <zlib.h>
#include <zconf.h>

#define SPNG_NOFILTER 0
#define SPNG_SUB 1
#define SPNG_UP 2
#define SPNG_AVG 3
#define SPNG_PAETH 4
#define SPNG_ERROR -1
#define SPNG_FILE_NOT_FOUND -2
#define SPNG_NOT_ALLOCATED -3
#define SPNG_NULL -4
#define SPNG_SUCCESS 0
#define SPNG_WRITTEN_AS_INDEXED 10

struct SPNG_PIXEL{
unsigned char r;
unsigned char g;
unsigned char b;
unsigned char a;
};


struct SPNG_AUTHORINFO{
char * author;
int author_len;
char * title;
int title_len;
char * description;
int description_len;
char * Copyright;
int Copyright_len;
char * Creation;
int Creation_len;
char * Software;
int Software_len;
char * Disclaimer;
int Disclaimer_len;
char * Warning;
int Warning_len;
char * Source;
int Source_len;
char * Comment;
int Comment_len;
};

struct SPNG_INFO {
unsigned int width; 
unsigned int height;
unsigned char bitdepth;
unsigned char clr; //Colortype ie RGB,RGBA,greyscale..
unsigned long size; //size of the pixelbuffer
unsigned char bytespp;// if greyscale 1 rgb 3 greyalpha 2 rgba 4
};

/*
Profile name 	1-79 bytes (character string)
Null separator 	1 byte (null character)
Compression method 	1 byte
Compressed profile 	n bytes
*/


extern const unsigned char g_spng_IDAT_ID[4];
extern const unsigned char g_spng_PLTE_ID[4];
extern const unsigned char g_spng_PNG_ID[8];
extern const unsigned char g_spng_TRNS_ID[4];
extern const unsigned char g_spng_tExT_ID[4];
extern const unsigned char g_spng_zTXt_ID[4];
extern const unsigned char g_spng_IHDR_ID[4];
extern const unsigned char g_spng_phys_ID[4];
extern const unsigned char g_spng_iTXt_ID[4];
extern const unsigned char g_spng_bkgd_ID[4];
extern const unsigned char g_spng_cHRM_ID[4];
extern const unsigned char g_spng_gAMA_ID[4];
extern const unsigned char g_spng_iCCP_ID[4];
extern const unsigned char g_spng_sRGB_ID[4];
extern const unsigned char g_spng_sBIT_ID[4];
extern const unsigned char g_spng_sPLT_ID[4];
extern const unsigned char g_spng_tIME_ID[4];
extern const unsigned char g_spng_hIST_ID[4];
extern const unsigned char g_spng_IHDR_ID[4];
extern const unsigned char g_spng_phys_ID[4];
extern unsigned int g_spng_is_allocated; //keeps track if it has been Initialized by the same program
extern unsigned int g_spng_plte_is_allocated; //keeps track if plte is initialized
extern unsigned int g_trns_len;
extern unsigned int g_spng_has_trns;

extern unsigned char * g_spng_iccp_raw; // all bytes of the found iccp
extern unsigned int g_spng_iccp_raw_len;
extern unsigned char g_spng_iccp_allocated;

extern struct SPNG_ICCP g_spng_iccp;

extern struct SPNG_INFO g_spng_spnginf; //keeps track of the currently loaded images dimensions,bytespp,clrtype ...
extern struct SPNG_PIXEL* g_spng_plte_pixels;
extern struct SPNG_AUTHORINFO g_spng_author_info;


extern unsigned int g_spng_plte_len;
extern  unsigned int g_spng_crc; // global crc val
extern  unsigned int g_spng_bytes_rwritten;
extern  unsigned char g_spng_is_little_endian;
extern unsigned char g_spng_bkgd[6];

#ifdef SPNGLIB_DEBUG_BENCHMARK
    #include <time.h>
    #include <stdio.h>
    extern  float SPNG_BENCH_START;
    extern  float SPNG_BENCH_END;
    extern  float SPNG_BENCH_ELAPSED;
    extern unsigned int SUB_COUNT;
    extern unsigned int UP_COUNT;
    extern unsigned int AVG_COUNT;
    extern unsigned int PAETH_COUNT;
    extern unsigned int NOFILTER_COUNT;
    void spng_bench_start();
    void spng_bench_end(char * endmsg);
    void dump_buffer_to_file(const char* buffer, size_t buffer_size, const char* file_name) ;

#endif