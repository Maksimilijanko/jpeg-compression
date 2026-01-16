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
#include "jfif_handler.h"

// --------------------------------------------------------------------------------
// Function Declaration
// --------------------------------------------------------------------------------
void send_image_to_c7x(RGB *original_rgb_data, int width, int height, uint8_t* result, uint32_t* result_size);
void image_to_blocks(RGB *image_buffer, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, RGB *out_blocks);

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
    // We need to reorder indexes to block-row-major order
    uint32_t blocks_w = (image.info.width + 7) / 8;             // ceiling division
    uint32_t blocks_h = (image.info.height + 7) / 8;
    RGB* block_order_pixels = (RGB*)calloc(blocks_w * blocks_h * 64, sizeof(RGB));
    image_to_blocks(pixels, image.info.width, image.info.height, &blocks_w, &blocks_h, block_order_pixels);

    
    // Check if image data exists
    if (image.buffer == NULL) {
        printf("ERROR: Failed to load image data.\n");
        appDeInit();
        return -1;
    }

    printf("BMP image imported.\n");
    printf("Original size: %u x %u\n", image.info.width, image.info.height);
    printf("File size: %u\n", image.header.file_size);

    uint8_t* buffer = (uint8_t*)calloc(image.info.width *  image.info.height, sizeof(uint8_t));
    uint32_t result_size;

    // Dispatch processing to C7x
    send_image_to_c7x(block_order_pixels, blocks_w * 8, blocks_h * 8, buffer, &result_size);

    FILE *f_out = fopen(params.outputFile, "wb");
    if(f_out) {
        write_to_jfif(f_out, buffer, result_size, image.info.width, image.info.height);
        fclose(f_out);
        printf("[A72] JFIF serialization completed.\n");
    }

    free(buffer);
    free(pixels);
    free(image.buffer);
    appDeInit();
    
    return 0;
}

void image_to_blocks(RGB *image_buffer, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, RGB *out_blocks) {
    uint32_t blocks_w, blocks_h;
    uint32_t by, bx, y, x;
    uint32_t img_x, img_y, block_index;

    blocks_w = (width + 7) / 8;             // ceiling division
    blocks_h = (height + 7) / 8;             

    *out_blocks_w = blocks_w;
    *out_blocks_h = blocks_h;

    for(by = 0; by < blocks_h; by++) {
        for(bx = 0; bx < blocks_w; bx++) {

            for(y = 0; y < 8; y++) {
                for(x = 0; x < 8; x++) {
                    img_x = bx * 8 + x < width ? bx * 8 + x : width - 1;
                    img_y = by * 8 + y < height ? by * 8 + y : height - 1;
                    block_index = (by * blocks_w + bx) * 64 + (y * 8 + x);
                    out_blocks[block_index] = image_buffer[img_y * width + img_x];
                }
            }
        }
    }

}

// --------------------------------------------------------------------------------
// A72 Logic to communicate with C7x
// --------------------------------------------------------------------------------
void send_image_to_c7x(RGB* original_rgb_data, int width, int height, uint8_t* result, uint32_t* result_size)
{
    uint32_t plane_size = width * height;
    uint32_t total_input_size = plane_size * 3;

    printf("[A72] Allocating shared memory...\n");

    // Allocate contiguous memory in DDR
    uint8_t *shared_input_virt = appMemAlloc(APP_MEM_HEAP_DDR, total_input_size, 64);
    uint8_t *shared_output_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size, 64);
    int8_t *shared_intermediate_1_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size, 64);
    int16_t *shared_intermediate_2_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size * sizeof(int16_t), 64);    // we make it 2x larger because quantized DCT coeffs are represented with 2B each.
    int16_t *shared_intermediate_3_virt = appMemAlloc(APP_MEM_HEAP_DDR, plane_size * sizeof(int16_t), 64); 
    float   *shared_intermediate_dct_buff = appMemAlloc(APP_MEM_HEAP_DDR, plane_size * sizeof(float), 64);

    if (!shared_input_virt || !shared_output_virt) {
        printf("[A72] Error: Memory allocation failed!\n");
        return;
    }

    // Separate planar pointers
    uint8_t *ptr_r = shared_input_virt;
    uint8_t *ptr_gb = shared_input_virt + plane_size;
    
    for(int i = 0, k = 0; i < plane_size; i += 32, k += 64) {
        for(int j = 0; j < 32; j++) {
            ptr_r[i + j] = original_rgb_data[i + j].r;
            
            ptr_gb[k + j] = original_rgb_data[i + j].g;
            ptr_gb[k + j + 32] = original_rgb_data[i + j].b;
        }
    }

    // DEBUG INFO
    printf("\n");
    for(int i = 1; i <= 64; i++)
    {
        printf("%d ", ptr_r[i]);
        if(i % 8 == 0)
            printf("\n");
    }
    printf("\n");
    for(int i = 1; i <= 64; i++)
    {
        printf("%d ", ptr_gb[i]);
        if(i % 8 == 0)
            printf("\n");
    }
    printf("\n");
 
    
    // CACHE WRITEBACK: Push data from A72 Cache to DDR so C7x can see it
    appMemCacheWb(shared_input_virt, total_input_size);

    // Prepare DTO
    JPEG_COMPRESSION_DTO packet;
    packet.width = width;
    packet.height = height;

    // Each processor has it's own virtual memory space. We need to translate virtual -> physical address.
    uint64_t input_phys_base = appMemGetVirt2PhyBufPtr((uint64_t)shared_input_virt, APP_MEM_HEAP_DDR);
    
    packet.phys_addr_r = input_phys_base;
    packet.phys_addr_gb = input_phys_base + plane_size;
    // packet.phys_addr_b = input_phys_base + (2 * plane_size);
    
    packet.phys_addr_y_out = appMemGetVirt2PhyBufPtr((uint64_t)shared_output_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_intermediate_1 = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_1_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_intermediate_2 = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_2_virt, APP_MEM_HEAP_DDR);
    packet.phys_addr_intermediate_3 = appMemGetVirt2PhyBufPtr((uint64_t)shared_intermediate_3_virt, APP_MEM_HEAP_DDR);
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
    appMemCacheInv(shared_intermediate_2_virt, plane_size * 2);
    appMemCacheInv(shared_intermediate_3_virt, plane_size * 2);
    appMemCacheInv(shared_intermediate_dct_buff, plane_size * 4);

    // Copy result to out buffer
    *result_size = packet.output_size;
    memcpy(result, shared_output_virt, packet.output_size);
    
    // Free Memory
    appMemFree(APP_MEM_HEAP_DDR, shared_input_virt, total_input_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_output_virt, plane_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_1_virt, plane_size);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_2_virt, plane_size * 2);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_3_virt, plane_size * 2);
    appMemFree(APP_MEM_HEAP_DDR, shared_intermediate_dct_buff, plane_size * 4);
}