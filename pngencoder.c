
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
   unsigned long PNG_update_crc(unsigned long crc, unsigned char *buf,
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
     return PNG_update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
    }

   uint8_t* png_crc_buf;
//Writes the Chunk length and IDAT identifier
void write_chnk_len_IDAT(unsigned int chnk_len, FILE* fp){
    unsigned int Chunk_length = chnk_len;
    Chunk_length = __builtin_bswap32(Chunk_length);
    fwrite(&Chunk_length,1,4,fp);
    fwrite(IDAT_ID, 1, 4, fp);
}

void PNG_make2d_Idat(uint8_t*** out,uint8_t** in,int w, int h){
    (*out) = (uint8_t**)malloc(h*sizeof(uint8_t*));
    int in_i = 0; 
    for(int i=0;i < h; i++ ){
        (*out)[i] = malloc(w*4);
        for(int j = 0 ; j < w*4 ; j++){
            (*out)[i][j] = (*in)[in_i];
            in_i++;
        }
    }
}

void PNG_mipmap(uint8_t** buf,uint8_t** scaled_out,int h, int w,int scale){
    pixel_pp pp;
    pixel_pp pp_sc;//scaled 2d pixel
   // PNG_make2d_Idat(&p_vals2d,buf,w,h);
    PNG_Get_Pixelpp(&pp);
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

unsigned long PNG_deflate(uint8_t *t, int ScanLineLen, int height, int bytepp,int blocksize, FILE *fp)
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
        for(int i  = bytes_pp ;i < len; i += bytes_pp){
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
        ScanLine[0] = PNG_calc_filterval(len,0);
        applyFilter(filtered_Scan,len);
        temp_i = copyScantoBuf(temp,filtered_Scan,len,temp_i);
        i+=len;
    }
    free(ScanLine);
    free(filtered_Scan);
    free(previous_ScanLine);
}

void Png_Encode(uint8_t *IDAT_input,  char *PngName, int width, int height, int bytepp,int clr_opt)
{
   // Exec_time_Start();
    bytespp = bytepp;
    struct IHDR_CHUNK ihdr;

    ihdr.height = height;
    ihdr.width = width; 
    ihdr.colour_type = clr_opt;
    ihdr.bit_depth = 8;

    unsigned long long ScanLineLen = (ihdr.width * bytespp) + 1;
    int Bufsize = ScanLineLen*ihdr.height;
    
    uint8_t *temp = malloc(Bufsize);
    //write_image_to_console(IDAT_input,&ihdr);
    int index = 0;
    int IDAT_i = 0;
    //this calculates the best filters and applies them
    makefilteredbuffer(IDAT_input,temp,Bufsize,ScanLineLen);
    FILE *fp = fopen(PngName, "wb");
    fwrite(png_identifier, 1, 8, fp);
    fwrite(standard_ihdr_length, 1, 4, fp);
    uint32_t w = 0;
    uint32_t h = 0;
    uint8_t b = 0; 
    b = 8;
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
    uint8_t* buf = malloc(17);    
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
    unsigned long e=0;
    unsigned long fp_p = ftell(fp)-8;
    

    e = PNG_deflate(temp, ScanLineLen, ihdr.height, bytespp,67536, fp);
    fwrite(zeroes,1,4,fp);
    fwrite(IDAT_END_ID, 1, 4, fp);    
    free(temp);
    free(IDAT_input);
    uint8_t EOI[] = {0xae, 0x42, 0x60, 0x82};
    fwrite(EOI, 1, 4, fp);
    fclose(fp);
   // Exec_time_stop();
}
//........ PNGENCODING .........//]
