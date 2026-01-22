#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>

#define NUM_BLOCKS 32

#ifdef DEBUG_CYCLE_COUNT
    static void format_commas(uint64_t n, char *out) {
        char temp[64];
        sprintf(temp, "%llu", n);
        
        int len = strlen(temp);
        int num_commas = (len - 1) / 3;
        int new_len = len + num_commas;
        
        out[new_len] = '\0'; 
        
        int src = len - 1;
        int dst = new_len - 1;
        int cnt = 0;
        
        while (src >= 0) {
            out[dst--] = temp[src--];
            if (++cnt == 3 && src >= 0) {
                out[dst--] = ','; 
                cnt = 0;
            }
        }
    }

    // global vars for calcuating dct performance
    uint64_t timer1;
    uint64_t timer2;
    uint64_t timer3;
    uint64_t total_fetch_time;
    uint64_t total_dct_time;
    uint64_t total_quantization_time;
    uint64_t total_zig_zag_time;
    uint64_t total_rle_pred_encoding_time;
    uint64_t total_huffman_time;
    uint64_t total_encoding_time;
    uint64_t start;
#endif

int32_t JpegCompression_RemoteServiceHandler(char *service_name, uint32_t cmd, void *prm, uint32_t prm_size, uint32_t flags)
{
    JPEG_COMPRESSION_DTO* packet = (JPEG_COMPRESSION_DTO*) prm;

    // Reset global variables
    // Global variables are alive as long as the service is up, which is determined by vision apps running 
    #ifdef DEBUG_CYCLE_COUNT
        total_fetch_time = 0;
        total_dct_time = 0;
        total_quantization_time = 0;
        total_zig_zag_time = 0;
        total_rle_pred_encoding_time = 0;
        total_huffman_time = 0;
        total_encoding_time = 0;
    #endif


    uint8_t *vec_r = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_r);
    uint8_t *vec_gb = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_gb);
    uint8_t *vec_y = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_y_out);

    uint64_t total_pixels = packet->width * packet->height;

    // Cache invalidation
    appMemCacheInv(vec_r, total_pixels);
    appMemCacheInv(vec_gb, total_pixels);

    uint32_t blocks_w, blocks_h, total_blocks;            // BLOCKS SIZE

    blocks_w = (packet->width + 7) / 8;             // ceiling division
    blocks_h = (packet->height + 7) / 8;     
    total_blocks = blocks_w * blocks_h;
    uint64_t i;

    uint16_t size = NUM_BLOCKS * 64;

    // Statically allocated stack buffers for processing a single block
    int8_t __attribute__((aligned(64))) block[size];
    float __attribute__((aligned(64))) dct_block[size];
    int16_t __attribute__((aligned(64))) quantized_dct[size];
    int16_t __attribute__((aligned(64))) zigzagged[size];

    // We are procesing num-blocks at once.
    // This could lead to memory unsafety, but because we are configuring SE with total_pixels
    // this will not happen!
    fetch_setup(vec_r, vec_gb, total_pixels);
    init_zigzag();

    BitWriter bw;
    // write the result into vec_y
    bw.buffer = vec_y;
    bw.byte_pos = 0;
    bw.bit_pos = 0;
    bw.current = 0;

    int16_t global_prev_dc = 0;

    for(i = 0; i < total_blocks; i += NUM_BLOCKS) {
        #ifdef DEBUG_CYCLE_COUNT
            start = __TSC;
        #endif

        fetch_next_blocks(block, NUM_BLOCKS);

        #ifdef DEBUG_CYCLE_COUNT
            total_fetch_time += __TSC - start;
            start = __TSC;
        #endif

        perform_dct_on_blocks(block, dct_block, NUM_BLOCKS);

        #ifdef DEBUG_CYCLE_COUNT
            total_dct_time += __TSC - start;
            start = __TSC;
        #endif

        quantize_block(dct_block, quantized_dct, NUM_BLOCKS);

        #ifdef DEBUG_CYCLE_COUNT
            total_quantization_time += __TSC - start;
            start = __TSC;
        #endif

        zigzag_order(quantized_dct, zigzagged, NUM_BLOCKS);

        #ifdef DEBUG_CYCLE_COUNT
            total_zig_zag_time += __TSC - start;
            start = __TSC;
        #endif
        
        encode_block_batch(zigzagged, &global_prev_dc, &bw, NUM_BLOCKS);

        #ifdef DEBUG_CYCLE_COUNT
            total_encoding_time += __TSC - start;
            start = __TSC;
        #endif

    }

    // clean out the remaining byte from BW
    flush_bits(&bw);

    // Write out output size so A72 can perform serialization
    packet->output_size = bw.byte_pos;

    // CACHE WRITEBACK (After processing)
    // Push data from cache to DDR so A72 can read it. 
    appMemCacheWb(vec_y, total_pixels);

    // Perform cycle calculation and print

    #ifdef DEBUG_CYCLE_COUNT
        uint64_t diff_total = total_fetch_time + total_dct_time + total_quantization_time + total_zig_zag_time + total_encoding_time;
              
        static char log_buf[2048]; 
        char fmt_buf[64];
        int offset = 0;
        
        const char* separator = "=======================================================================\n";
        const char* divider   = "-----------------------------------------------------------------------\n";

        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "\n");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "%s", separator);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "JPEG COMPRESSION STAGE", "CYCLES");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "%s", separator);

        format_commas(total_fetch_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "RGB -> Y Conversion (segm, Y conv, cent.)", fmt_buf);
        
        format_commas(total_dct_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "DCT Transform (Total)", fmt_buf);

        format_commas(total_quantization_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "Quantization", fmt_buf);
        
        format_commas(total_zig_zag_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "ZigZag Reorder", fmt_buf);
        
        format_commas(total_encoding_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "Huffman Encoding", fmt_buf);

        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "%s", divider);
        
        format_commas(diff_total, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "TOTAL CYCLES", fmt_buf);
        
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "%s\n", separator);

        appLogPrintf("%s", log_buf);

    #endif
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
