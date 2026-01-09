#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

// TI Vision Apps Headers
#include <TI/tivx.h>
#include <utils/ipc/include/app_ipc.h>             
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/app_init/include/app_init.h>     
#include <utils/console_io/include/app_log.h>
#include <utils/mem/include/app_mem.h> 

// Project Headers
#include "jpeg_compression.h"
#include "bmp_handler.h"
#include "color_spaces.h"

// --------------------------------------------------------------------------------
// Function Declaration
// --------------------------------------------------------------------------------
void send_image_to_c7x(RGB *original_rgb_data, int width, int height);

// --------------------------------------------------------------------------------
// Main Function
// --------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // SYSTEM INIT (Initializes IPC, Memory, and Logs)
    appLogPrintf("JPEG: Initializing App...\n");
    int32_t status = appInit();
    if(status != 0) {
        printf("ERROR: appInit failed!\n");
        return -1;
    }

    PARAMETERS params = parse_parameters(argc, argv);

    // Load BMP
    BMP_IMAGE image = load_bmp_image(params.inputFile); 
    RGB* pixels = read_pixels(image.buffer, image.info.width, image.info.height, image.info.height <= 0);
    
    // Check if image data exists
    if (image.buffer == NULL) {
        printf("ERROR: Failed to load image data.\n");
        appDeInit();
        return -1;
    }

    printf("BMP image imported.\n");
    printf("Original size: %u x %u\n", image.info.width, image.info.height);
    printf("File size: %u\n", image.header.file_size);

    // Dispatch processing to C7x
    send_image_to_c7x(pixels, image.info.width, image.info.height);

    appDeInit();
    
    return 0;
}

// --------------------------------------------------------------------------------
// A72 Logic to communicate with C7x
// --------------------------------------------------------------------------------
void send_image_to_c7x(RGB* original_rgb_data, int width, int height)
{
    uint32_t plane_size = width * height;
    uint32_t total_input_size = plane_size * 3;

    printf("[A72] Allocating shared memory...\n");

    // Allocate contiguous memory in DDR
    uint8_t *shared_input_virt = appMemAlloc(APP_MEM_HEAP_DDR, total_input_size, 64);
    uint8_t *shared_output_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size, 64);
    uint8_t *shared_intermediate_1_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size, 64);
    int16_t *shared_intermediate_2_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size * sizeof(int16_t), 64);    // we make it 2x larger because quantized DCT coeffs are represented with 2B each.
    float   *shared_intermediate_dct_buff = appMemAlloc(APP_MEM_HEAP_DDR, plane_size * sizeof(float), 64);

    if (!shared_input_virt || !shared_output_virt) {
        printf("[A72] Error: Memory allocation failed!\n");
        return;
    }

    // Separate planar pointers
    uint8_t *ptr_r = shared_input_virt;
    uint8_t *ptr_g = shared_input_virt + plane_size;
    uint8_t *ptr_b = shared_input_virt + (2 * plane_size);

    // De-interleave pixel data (RGB -> R, G, B)
    for(int i = 0; i < plane_size; i++) {
        ptr_b[i] = original_rgb_data[i].b;
        ptr_g[i] = original_rgb_data[i].g;
        ptr_r[i] = original_rgb_data[i].r;
    }

    // CACHE WRITEBACK: Push data from A72 Cache to DDR so C7x can see it
    appMemCacheWb(shared_input_virt, total_input_size);

    // Prepare DTO
    JPEG_COMPRESSION_DTO packet;
    packet.width = width;
    packet.height = height;

    // Each processor has it's own virtual memory space. We need to translate virtual -> physical address.
    uint64_t input_phys_base = appMemGetVirt2PhyBufPtr((uint64_t)shared_input_virt, APP_MEM_HEAP_DDR);
    
    packet.phys_addr_r = input_phys_base;
    packet.phys_addr_g = input_phys_base + plane_size;
    packet.phys_addr_b = input_phys_base + (2 * plane_size);
    
    packet.phys_addr_y_out = appMemGetVirt2PhyBufPtr((uint64_t)shared_output_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_intermediate_1 = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_1_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_intermediate_2 = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_2_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_dct_buff = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_dct_buff, APP_MEM_HEAP_DDR);

    
    printf("[A72] Sending IPC message to C7x...\n");
    printf("[A72] Targeting service: %s\n", JPEG_COMPRESSION_REMOTE_SERVICE_NAME);

    // CALL REMOTE SERVICE
    int32_t status = appRemoteServiceRun(
        APP_IPC_CPU_C7x_1,                     
        JPEG_COMPRESSION_REMOTE_SERVICE_NAME,  
        0,                  
        &packet,                               
        sizeof(packet),                       
        0                                      
    );

    if (status != 0) {
        printf("[A72] Error: Remote service call failed with status %d\n", status);
    } else {
        printf("[A72] Remote service success!\n");
    }

    // CACHE INVALIDATE: Pull data from DDR to A72 Cache to see what C7x wrote
    appMemCacheInv(shared_output_virt, plane_size);
    appMemCacheInv(shared_intermediate_1_virt, plane_size);
    appMemCacheInv(shared_intermediate_2_virt, plane_size);
    appMemCacheInv(shared_intermediate_dct_buff, plane_size);

    // DEBUG: Print results (First 100 pixels of Y plane)
    int8_t *y_result = (int8_t*) shared_output_virt;
    printf("[A72] Result Y Plane (First 100 pixels):\n");
    for(int i = 0; i < 64; i++) {
        printf("%3d\t", y_result[i]);

        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    float *dct_result = (float*) shared_intermediate_dct_buff;
    printf("[A72] Result DCT coeffs  (First 100 pixels):\n");
    for(int i = 0; i < 64; i++) {
        printf("%.3f\t", dct_result[i]);

        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    int16_t *dct_result_q = shared_intermediate_2_virt;
    printf("[A72] Result DCT coeffs (quantized) (First 100 pixels):\n");
    for(int i = 0; i < 64; i++) {
        printf("%3d\t", dct_result_q[i]);

        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    
    // Free Memory
    appMemFree(APP_MEM_HEAP_DDR, shared_input_virt, total_input_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_output_virt, plane_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_1_virt, plane_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_2_virt, plane_size * 2);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_dct_buff, plane_size * 4);
}
