#ifndef JPEG_COMPRESSION_H
#define JPEG_COMPRESSION_H

// ============================================================================
// Common header files (Standard C)
// ============================================================================
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// ============================================================================
// C7x-specific headers
// ============================================================================
#ifdef __C7000__
    #include <stdio.h>
    #include <string.h>
    #include <assert.h>
    #include <stdint.h>
    #include <TI/tivx.h>
    #include <utils/ipc/include/app_ipc.h>
    #include <utils/remote_service/include/app_remote_service.h>
    #include <utils/console_io/include/app_log.h>
    #include <utils/mem/include/app_mem.h>
    #include <c7x.h>
#else
// ========================================================================
// A72 (ARM) TYPES COMPATIBILITY
// ========================================================================
    typedef uint8_t uchar64;
#endif

#define JPEG_COMPRESSION_REMOTE_SERVICE_NAME "org.etfbl.jpegcomp"

// ============================================================================
// COMMONS STRUCTURES (USED IN HOST-DEVICE COMMUNICATION)
// ============================================================================

#define PI_VAL 3.14159265358979323846f

/*
* Structure definitions. These are used when passing data via IPC.
*/

typedef struct
{
    int32_t width;                          // image dimensions
    int32_t height;
    
    uint64_t phys_addr_r;                   // R planar
    uint64_t phys_addr_g;                   // G planar
    uint64_t phys_addr_b;                   // B planar
    uint64_t phys_addr_intermediate_1;      // a buffer to write intermediate results on C7x side (we perform allocation on host/A72 side)
    uint64_t phys_addr_intermediate_2;    
    uint64_t phys_addr_dct_buff;
    uint64_t phys_addr_y_out;               // return value
} JPEG_COMPRESSION_DTO;


// ============================================================================
// C7x specific functions
// ============================================================================
#ifdef __C7000__
    extern uint8_t std_lum_qt[64];          // quantization table declaration (defined in quantization_table.c)

    // Remote service handler
    int32_t JpegCompression_RemoteServiceHandler(char *service_name, uint32_t cmd,
                                                 void *prm, uint32_t prm_size, uint32_t flags);

    // Service initialization function
    int32_t JpegCompression_Init();

    // Convert RGB color space to Y
    void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, uint8_t *y_ptr, int num_pixels);

    void perform_dct_on_block(uint8_t *b_start, float *dct_coeffs);

    void image_to_blocks(uint8_t *image_buffer, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, uint8_t *out_blocks);

    void quantize_block(float *dct_block, int16_t* out_quantized_block);

#endif 

#endif 
