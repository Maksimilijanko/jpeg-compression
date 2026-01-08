#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmp_handler.h"

BMP_IMAGE load_bmp_image(const char* inputFile) {
    BMP_IMAGE image = {0}; 
    FILE *fin = fopen(inputFile, "rb");
    
    if (fin == NULL) {
        printf("Error: Cannot open file %s\n", inputFile);
        return image;
    }

    // Load file header (14 bytes)
    if (fread(&image.header, sizeof(BMP_FILE_HEADER), 1, fin) != 1) {
        printf("Error: File is too short for BMP header.\n");
        fclose(fin);
        return image;
    }

    if (image.header.file_type != 0x4D42) { // 'BM'
        printf("Error: File is not a BMP format (Magic number mismatch).\n");
        fclose(fin);
        return image;
    }

    // Load info header (at least 40 bytes)
    // All BMPs have the same first 40 bytes of info header
    // If there are extra bytes, they are ignored
    if (fread(&image.info, sizeof(BMP_INFO), 1, fin) != 1) {
         printf("Error: Cannot read Info header.\n");
         fclose(fin);
         return image;
    }

    // Calculate data size
    // Data size can be calculated as difference between file size and pixel data offset
    // Data offset differs based on headers size
    uint32_t dataSize = image.header.file_size - image.header.offset;

    // Memory allocation for pixel data
    image.buffer = (unsigned char*)malloc(dataSize);
    if (image.buffer == NULL) {
        printf("Error: Not enough memory (Requested: %d bytes).\n", dataSize);
        fclose(fin);
        return image; 
    }

    // Level file pointer to pixel data
    // We've read 14 + 40 bytes so far, but there might be extra header bytes
    // Therefore, we seek to the offset specified in the file header
    fseek(fin, image.header.offset, SEEK_SET);
    
    // Read pixel data
    size_t bytesRead = fread(image.buffer, 1, dataSize, fin);
    if (bytesRead != dataSize) {
        printf("Warning: Expected %d bytes, read %d.\n", dataSize, (int)bytesRead);
    }

    image.info.img_size = dataSize;

    fclose(fin);
    return image;
}


void store_bmp_image(BMP_IMAGE image, const char* outputFile) {
    FILE *fout = fopen(outputFile, "wb");
    if (fout == NULL) {
        printf("Error while opening output file.\n");
        return;
    }
    
    // Update headers before writing
    // Input image might've had different header sizes which affected offsets
    // We standardize to 14 + 40 bytes headers, so we need to update offsets and sizes
    image.info.size = sizeof(BMP_INFO); 
    image.header.offset = sizeof(BMP_FILE_HEADER) + sizeof(BMP_INFO);
    image.header.file_size = image.header.offset + image.info.img_size;
    image.info.compression = 0; 

    fwrite(&image.header, sizeof(BMP_FILE_HEADER), 1, fout);
    fwrite(&image.info, sizeof(BMP_INFO), 1, fout);
    
    fwrite(image.buffer, 1, image.info.img_size, fout);

    fclose(fout);
    printf("Image saved: %s\n", outputFile);
}

void free_bmp_image(BMP_IMAGE image) {
    if (image.buffer != NULL) {
        free(image.buffer);
    }
}

PARAMETERS parse_parameters(int argc, char* argv[]) {
    PARAMETERS params = {NULL, NULL};
    for(int i = 0; i < argc; i++) {
        if(strcmp("-output", argv[i]) == 0 && i + 1 < argc) {
            params.outputFile = argv[++i];
        }
        else if(strcmp("-input", argv[i]) == 0 && i + 1 < argc) {
            params.inputFile = argv[++i];
        }
    }
    return params;
}
