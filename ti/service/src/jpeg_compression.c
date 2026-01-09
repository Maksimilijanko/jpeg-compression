#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>

int32_t JpegCompression_RemoteServiceHandler(char *service_name, uint32_t cmd, void *prm, uint32_t prm_size, uint32_t flags)
{
    JPEG_COMPRESSION_DTO* packet = (JPEG_COMPRESSION_DTO*) prm;


    uint8_t *vec_r = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_r);
    uint8_t *vec_g = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_g);
    uint8_t *vec_b = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_b);
    uint8_t *vec_y = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_y_out);

    int total_pixels = packet->width * packet->height;

    // Cache invalidation
    appMemCacheInv(vec_r, total_pixels);
    appMemCacheInv(vec_g, total_pixels);
    appMemCacheInv(vec_b, total_pixels);

        
    rgb_to_y( vec_r, vec_g, vec_b, vec_y, total_pixels);

    // CACHE WRITEBACK (After processing)
    // Push data from cache to DDR so A72 can read it. 
    appMemCacheWb(vec_y, total_pixels);
    

    return 0;
}

int32_t JpegCompression_Init()
{
    int32_t status = -1;
    appLogPrintf("JPEG Compression Service: Init ... !!!");
    status = appRemoteServiceRegister(JPEG_COMPRESSION_REMOTE_SERVICE_NAME, JpegCompression_RemoteServiceHandler);
    if(status != 0)
    {
        appLogPrintf("JPEG Compression Service: ERROR: Unable to register remote service handler\n");
    }
    appLogPrintf("JPEG Compression Service: Init ... DONE!!! (service name: %s\n", JPEG_COMPRESSION_REMOTE_SERVICE_NAME);
    return status;
}


void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, uint8_t *y_ptr, int num_pixels)
{
    int32_t num_vectors = num_pixels / 32;
    int32_t i; 

    uchar32 * restrict vec_r = (uchar32 *) r_ptr;
    uchar32 * restrict vec_g = (uchar32 *) g_ptr;
    uchar32 * restrict vec_b = (uchar32 *) b_ptr;
    char32 * restrict vec_y = (char32 *) y_ptr;

    short32 coeff_r = (short32) 77;
    short32 coeff_g = (short32) 150;
    short32 coeff_b = (short32) 29;
    
    short32 val_128 = (short32) 128;

    for (i = 0; i < num_vectors; i++) {
        uchar32 r_in = vec_r[i];
        uchar32 g_in = vec_g[i];
        uchar32 b_in = vec_b[i];

        short32 r_s = convert_short32(r_in);
        short32 g_s = convert_short32(g_in);
        short32 b_s = convert_short32(b_in);

        short32 y_temp = (r_s * coeff_r) + (g_s * coeff_g) + (b_s * coeff_b);

        y_temp = y_temp >> 8;
        y_temp = y_temp - val_128;

        vec_y[i] = convert_char32(y_temp);
    }
}
