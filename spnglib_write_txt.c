#include "include/spnglib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int spng_deflate_txt(FILE * fp,char * data,unsigned char * dest_buf,int* data_len){
    if(fp == NULL) return SPNG_NULL;
    const size_t CHUNK_SIZE = 16384;
    int ret;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in =(* data_len);
    strm.next_in = (Bytef*)data;
    strm.avail_out = CHUNK_SIZE;
    strm.next_out = (Bytef*)dest_buf;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) return ret;
    ret = deflate(&strm, Z_FINISH);
    (*data_len )= strm.total_out;
    deflateEnd(&strm);
    return Z_OK;
}

// Writes a given keyword and its data after also writing the chunk id
int spng_write_keyword_data(FILE * fp,const unsigned char* ID,char * keyword,int keyword_len,char * data,int data_len,unsigned char should_be_compressed){
    if(fp == NULL) return SPNG_NULL;
    unsigned int temp_len = 0;
    if(should_be_compressed){
        unsigned char dest_buf[16384];
        spng_deflate_txt(fp,data,dest_buf,&data_len);
        temp_len = data_len+keyword_len+1;
        if(g_spng_is_little_endian)temp_len = __builtin_bswap32(temp_len);
        fwrite(&temp_len,1,4,fp);
        fwrite(ID,1,4,fp);
        fwrite(keyword,1,keyword_len,fp );
        fputc('\0',fp);
        fwrite(dest_buf,1,data_len,fp);
        g_spng_crc = crc32(0L, Z_NULL, 0L);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)keyword, keyword_len);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)'\0', 1);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)dest_buf, data_len);
        if(g_spng_is_little_endian)g_spng_crc = __builtin_bswap32(g_spng_crc);
        fwrite(&g_spng_crc,1,4,fp);
    }    
    else{
        temp_len = data_len+keyword_len+1;
        if(g_spng_is_little_endian)temp_len = __builtin_bswap32(temp_len);
        fwrite(&temp_len,1,4,fp);
        fwrite(ID,1,4,fp);
        fwrite(keyword,1,keyword_len,fp );
        fputc('\0',fp);
        g_spng_crc = crc32(0L, Z_NULL, 0L);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)keyword, keyword_len);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)'\0', 1);
        fwrite(data,1,data_len,fp);
        g_spng_crc = crc32(g_spng_crc, (const uint8_t*)data, data_len);
        if(g_spng_is_little_endian) g_spng_crc = __builtin_bswap32(g_spng_crc);
        fwrite(&g_spng_crc,1,4,fp);
    }
    return 1;
}

int SPNG_write_authorinfo(FILE * fp,struct SPNG_AUTHORINFO spngauthinf){
    if(fp == NULL) return SPNG_NULL;
    if(spngauthinf.title_len > 0)spng_write_keyword_data(fp,g_spng_tExT_ID,"Title",5,spngauthinf.title,spngauthinf.title_len,0);
    if(spngauthinf.author_len > 0)spng_write_keyword_data(fp,g_spng_tExT_ID,"Author",6,spngauthinf.author,spngauthinf.author_len,0);
    if(spngauthinf.description_len > 0)spng_write_keyword_data(fp,g_spng_zTXt_ID,"Description",11,spngauthinf.description,spngauthinf.description_len,1);
    if(spngauthinf.Comment_len > 0)spng_write_keyword_data(fp,g_spng_tExT_ID,"Comment",7,spngauthinf.Comment,spngauthinf.Comment_len,1);
    if(spngauthinf.Disclaimer_len > 0)spng_write_keyword_data(fp,g_spng_zTXt_ID,"Disclaimer",10,spngauthinf.Disclaimer,spngauthinf.Disclaimer_len,1);
    if(spngauthinf.Creation_len > 0)spng_write_keyword_data(fp,g_spng_zTXt_ID,"Creation Time",13,spngauthinf.Creation,spngauthinf.Creation_len,1);
    if(spngauthinf.Copyright_len > 0)spng_write_keyword_data(fp,g_spng_zTXt_ID,"Copyright",9,spngauthinf.Copyright,spngauthinf.Copyright_len,1);
    if(spngauthinf.Software_len > 0)spng_write_keyword_data(fp,g_spng_tExT_ID, "Software",8,spngauthinf.Software,spngauthinf.Software_len,0);
    if(spngauthinf.Source_len > 0)spng_write_keyword_data(fp,g_spng_tExT_ID,"Source",6,spngauthinf.Source,spngauthinf.Source_len,0);
    if(spngauthinf.Warning_len > 0)spng_write_keyword_data(fp,g_spng_zTXt_ID,"Warning",7,spngauthinf.Warning,spngauthinf.Warning_len,1);
    // TODO: Write Documentation
    // TODO: add reading EXIF
    return SPNG_SUCCESS;
}