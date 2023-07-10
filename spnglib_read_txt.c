#include "include/spnglib.h"
#include <stdlib.h>
#include <string.h>

int spng_deflate_ztxt(unsigned char * buffer,unsigned char  out[16384],unsigned int chnklen,unsigned int* out_length){
    int zlib_err;
    int out_len;
	  z_stream stream;
    // initialize zlib stream
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = (uInt)chnklen;
    stream.next_in = (Bytef*)buffer;
    stream.avail_out =(uInt)16384;
    stream.next_out = (Bytef*)(out);
    // decompress data using zlib
	zlib_err = inflateInit(&stream);
	zlib_err = inflate(&stream,Z_NO_FLUSH);
    (*out_length) = stream.total_out;
	inflateEnd(&stream);
    // allocate output buffer
    return zlib_err;
}
void SPNG_reset_author_info_lengths(struct SPNG_AUTHORINFO* info) {
    info->author_len = 0;
    info->title_len = 0;
    info->description_len = 0;
    info->Copyright_len = 0;
    info->Creation_len = 0;
    info->Software_len = 0;
    info->Disclaimer_len = 0;
    info->Warning_len = 0;
    info->Source_len = 0;
    info->Comment_len = 0;
}

void SPNG_free_author_info(struct SPNG_AUTHORINFO* info) {
    if (info->author_len > 0) {
        free(info->author);
        info->author_len = 0;
    }
    if (info->title_len > 0) {
        free(info->title);
        info->title_len = 0;
    }
    if (info->description_len > 0) {
        free(info->description);
        info->description_len = 0;
    }
    if (info->Copyright_len > 0) {
        free(info->Copyright);
        info->Copyright_len = 0;
    }
    if (info->Creation_len > 0) {
        free(info->Creation);
        info->Creation_len = 0;
    }
    if (info->Software_len > 0) {
        free(info->Software);
        info->Software_len = 0;
    }
    if (info->Disclaimer_len > 0) {
        free(info->Disclaimer);
        info->Disclaimer_len = 0;
    }
    if (info->Warning_len > 0) {
        free(info->Warning);
        info->Warning_len = 0;
    }
    if (info->Source_len > 0) {
        free(info->Source);
        info->Source_len = 0;
    }
    if (info->Comment_len > 0) {
        free(info->Comment);
        info->Comment_len = 0;
    }
}


void SPNG_get_Authorinfo(struct SPNG_AUTHORINFO* dest){
    SPNG_reset_author_info_lengths(dest);
    if (g_spng_author_info.author != NULL) {
        dest->author = (char *)malloc(g_spng_author_info.author_len + 1);
        dest->author_len = g_spng_author_info.author_len;
        memcpy(dest->author, g_spng_author_info.author, g_spng_author_info.author_len + 1);
    }
    if (g_spng_author_info.title != NULL) {
        dest->title = (char *)malloc(g_spng_author_info.title_len + 1);
        dest->title_len = g_spng_author_info.title_len;
        memcpy(dest->title, g_spng_author_info.title, g_spng_author_info.title_len + 1);
    }
    if (g_spng_author_info.description != NULL) {
        dest->description = (char *)malloc(g_spng_author_info.description_len + 1);
        dest->description_len = g_spng_author_info.description_len;
        memcpy(dest->description, g_spng_author_info.description, g_spng_author_info.description_len + 1);
    }
    if (g_spng_author_info.Copyright != NULL) {
        dest->Copyright = (char *)malloc(g_spng_author_info.Copyright_len + 1);
        memcpy(dest->Copyright, g_spng_author_info.Copyright, g_spng_author_info.Copyright_len + 1);
    }
    if (g_spng_author_info.Creation != NULL) {
        dest->Creation = (char *)malloc(g_spng_author_info.Creation_len + 1);
        memcpy(dest->Creation, g_spng_author_info.Creation, g_spng_author_info.Creation_len + 1);
    }
    if (g_spng_author_info.Software != NULL) {
        dest->Software = (char *)malloc(g_spng_author_info.Software_len + 1);
        memcpy(dest->Software, g_spng_author_info.Software, g_spng_author_info.Software_len + 1);
    }
    if (g_spng_author_info.Disclaimer != NULL) {
        dest->Disclaimer =(char *) malloc(g_spng_author_info.Disclaimer_len + 1);
        memcpy(dest->Disclaimer, g_spng_author_info.Disclaimer, g_spng_author_info.Disclaimer_len + 1);
    }
    if (g_spng_author_info.Warning != NULL) {
        dest->Warning = (char *)malloc(g_spng_author_info.Warning_len + 1);
        memcpy(dest->Warning, g_spng_author_info.Warning, g_spng_author_info.Warning_len + 1);
    }
    if (g_spng_author_info.Source != NULL) {
        dest->Source = (char *)malloc(g_spng_author_info.Source_len + 1);
        memcpy(dest->Source, g_spng_author_info.Source, g_spng_author_info.Source_len + 1);
    }
    if (g_spng_author_info.Comment != NULL) {
        dest->Comment =(char *) malloc(g_spng_author_info.Comment_len + 1);
        memcpy(dest->Comment, g_spng_author_info.Comment, g_spng_author_info.Comment_len + 1);
    }
}

/*
Title 	Short (one line) title or caption for image
Author 	Name of image's creator
Description 	Description of image (possibly long)
Copyright 	Copyright notice
Creation Time 	Time of original image creation
Software 	Software used to create the image
Disclaimer 	Legal disclaimer
Warning 	Warning of nature of content
Source 	Device used to create the image
Comment 	Miscellaneous comment

*/
void spng_alloc_metadata(char ** out, int * out_len, int in_len,char * in){
//if(in_len > 0) free((*out));
(*out) = (char*)malloc(in_len+1);
(*out_len) = in_len;
memcpy((*out),in,in_len);
(*out)[in_len] = '\0';
}

void spng_parse_std_metadata(char * data,unsigned int data_len,char * keyword,struct SPNG_AUTHORINFO* spng_auth){
    // just put the data  into the corresponding buffers in spng_auth
    // also check if it is already allocced before allocing again

    if(strcmp(keyword, "Title") == 0){
        spng_alloc_metadata(&spng_auth->title,&spng_auth->title_len,data_len,data);
    }
    else if(strcmp(keyword, "Description") == 0){
        spng_alloc_metadata(&spng_auth->description,&spng_auth->description_len,data_len,data);
    }
    else if(strcmp(keyword, "Author") == 0){
        spng_alloc_metadata(&spng_auth->author,&spng_auth->author_len,data_len,data);
    }
    else if(strcmp(keyword, "Copyright") == 0){
        spng_alloc_metadata(&spng_auth->Copyright,&spng_auth->Copyright_len,data_len,data);
    }
    else if(strcmp(keyword, "CreationTime") == 0){
        spng_alloc_metadata(&spng_auth->Creation,&spng_auth->Creation_len,data_len,data);
    }
    else if(strcmp(keyword, "Software") == 0){
        spng_alloc_metadata(&spng_auth->Software,&spng_auth->Software_len,data_len,data);
    }
    else if(strcmp(keyword, "Disclaimer") == 0){
        spng_alloc_metadata(&spng_auth->Disclaimer,&spng_auth->Disclaimer_len,data_len,data);
    }
    else if(strcmp(keyword, "Warning") == 0){
        spng_alloc_metadata(&spng_auth->Warning,&spng_auth->Warning_len,data_len,data);
    }
    else if(strcmp(keyword, "Source") == 0){
        spng_alloc_metadata(&spng_auth->Source,&spng_auth->Source_len,data_len,data);
    }
    else if(strcmp(keyword, "Comment") == 0){
        spng_alloc_metadata(&spng_auth->Comment,&spng_auth->Comment_len,data_len,data);
    }
}

void spng_read_author_title_desc(FILE * fp,int chnklen,int iscompressed,struct SPNG_AUTHORINFO* spng_auth){
    unsigned char buffer[chnklen];
    unsigned char* ztxt_buffer = (unsigned char *)malloc(16384);
    unsigned char ztxt_alloced=0;
    fread(buffer,1,chnklen,fp);
    if(iscompressed){
        unsigned int outlen=0;
        char  t[82];
        int i = 0;
        while(buffer[i] != '\0' && i < 82){
            i++;
        }
        t[i] = '\0';
        memcpy(t, buffer,i);
        int c = spng_deflate_ztxt( &buffer[i+2],ztxt_buffer,chnklen-i+2,&outlen);
        spng_parse_std_metadata((char *)ztxt_buffer,outlen,t,spng_auth);
    }else{
        int i = 0;
        char  t[82];
        while(buffer[i] != '\0' && i < 82){
            i++;
        }
        memcpy(t, buffer,i );
        t[i] = '\0';
        spng_parse_std_metadata((char *)&buffer[i+1],chnklen-i-1,t,spng_auth);
    }
    free(ztxt_buffer);
}

void spng_read_misc_text(){

}