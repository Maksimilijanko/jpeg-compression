#ifndef BMP_HANDLER_H
#define BMP_HANDLER_H

#include <stdint.h> 

// Ensure no padding in structures, because no padding is used in BMP file format
#pragma pack(push, 1) 

typedef struct {
    uint16_t file_type;      
    uint32_t file_size;      
    uint16_t reserved1;      
    uint16_t reserved2;      
    uint32_t offset;         
} BMP_FILE_HEADER;           

typedef struct {
    uint32_t size;           
    int32_t width;           
    int32_t height;          
    uint16_t planes;
    uint16_t bit_per_px;     
    uint32_t compression;    
    uint32_t img_size;       
    int32_t x_px_m;
    int32_t y_px_m;
    uint32_t clr_used;
    uint32_t clr_important;
} BMP_INFO;

#pragma pack(pop) 

typedef struct {
    BMP_FILE_HEADER header;
    BMP_INFO info;
    unsigned char *buffer; 
} BMP_IMAGE;

typedef struct {
    char* inputFile;
    char* outputFile;
} PARAMETERS;

BMP_IMAGE load_bmp_image(const char* inputFile);
void store_bmp_image(BMP_IMAGE image, const char* outputFile);
void free_bmp_image(BMP_IMAGE image);
PARAMETERS parse_parameters(int argc, char* argv[]);

#endif
