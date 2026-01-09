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

/*
* Structure definitions. These are used when passing data via IPC.
*/

typedef struct
{
    int32_t width;
    int32_t height;
    
    uint64_t phys_addr_r;
    uint64_t phys_addr_g;
    uint64_t phys_addr_b;

    uint64_t phys_addr_y_out;
} JPEG_COMPRESSION_DTO;


// ============================================================================
// C7x specific functions
// ============================================================================
#ifdef __C7000__

    // Remote service handler
    int32_t JpegCompression_RemoteServiceHandler(char *service_name, uint32_t cmd,
                                                 void *prm, uint32_t prm_size, uint32_t flags);

    // Service initialization function
    int32_t JpegCompression_Init();

    // Convert RGB color space to Y
    void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, uint8_t *y_ptr, int num_pixels);

#endif 

#endif 
