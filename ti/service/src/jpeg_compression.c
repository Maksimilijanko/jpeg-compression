#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>

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
    int8_t *vec_interm_buffer_1 = (int8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_1);
    int16_t *vec_interm_buffer_2 = (int16_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_2);
    int16_t *vec_interm_buffer_3 = (int16_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_3);
    float   *vec_dct_buff = (float *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_dct_buff);

    uint64_t total_pixels = packet->width * packet->height;

    // Cache invalidation
    appMemCacheInv(vec_r, total_pixels);
    appMemCacheInv(vec_gb, total_pixels);

    uint32_t blocks_w, blocks_h, total_blocks;            // BLOCKS SIZE

    blocks_w = (packet->width + 7) / 8;             // ceiling division
    blocks_h = (packet->height + 7) / 8;     
    total_blocks = blocks_w * blocks_h;
    uint64_t i;

    uint8_t num_blocks = 32;
    uint16_t size = num_blocks * 64;

    // Statically allocated stack buffers for processing a single block
    int8_t __attribute__((aligned(64))) block[size];
    float __attribute__((aligned(64))) dct_block[size];
    int16_t __attribute__((aligned(64))) quantized_dct[size];
    int16_t __attribute__((aligned(64))) zigzagged[size];

    // Control how many blocks are being processed at once
    

    fetch_setup(vec_r, vec_gb, total_pixels);
    init_zigzag();

    for(i = 0; i < total_blocks; i += num_blocks) {
        #ifdef DEBUG_CYCLE_COUNT
            start = __TSC;
        #endif

        fetch_next_blocks(block, num_blocks);

        #ifdef DEBUG_CYCLE_COUNT
            total_fetch_time += __TSC - start;
            start = __TSC;
        #endif

        perform_dct_on_blocks(block, dct_block, num_blocks);

        #ifdef DEBUG_CYCLE_COUNT
            total_dct_time += __TSC - start;
            start = __TSC;
        #endif


        // perform quantization on two blocks at once
        quantize_block(dct_block, quantized_dct, num_blocks);

        #ifdef DEBUG_CYCLE_COUNT
            total_quantization_time += __TSC - start;
            start = __TSC;
        #endif

        zigzag_order(quantized_dct, zigzagged, num_blocks);

        #ifdef DEBUG_CYCLE_COUNT
            total_zig_zag_time += __TSC - start;
            start = __TSC;
        #endif

        // copy it to one big intermediate buffer for performing encoding
        memcpy(vec_interm_buffer_3 + i * 64, zigzagged, 64 * 2 * num_blocks);        // copy two blocks of uint16_t

        #ifdef DEBUG_CYCLE_COUNT
            // total_quantization_time += __TSC - start;
            start = __TSC;
        #endif
    }

    BitWriter bw;
    // write the result into vec_y
    bw.buffer = vec_y;
    bw.byte_pos = 0;
    bw.bit_pos = 0;
    bw.current = 0;

    #ifdef DEBUG_CYCLE_COUNT
        start = __TSC;
    #endif


    int16_t prev_dc = 0;
    for(i = 0; i < total_blocks; i++) {
        prev_dc = encode_coefficients(vec_interm_buffer_3 + (i * 64), prev_dc, &bw);
    }

    // clean out the remaining byte from BW
    flush_bits(&bw);

    // Write out output size so A72 can perform serialization
    packet->output_size = bw.byte_pos;

    #ifdef DEBUG_CYCLE_COUNT
        total_encoding_time = __TSC - start;
        start = __TSC;
    #endif

    // CACHE WRITEBACK (After processing)
    // Push data from cache to DDR so A72 can read it. 
    appMemCacheWb(vec_y, total_pixels);
    appMemCacheWb(vec_interm_buffer_1, total_pixels);
    appMemCacheWb(vec_interm_buffer_2, total_pixels * 2);           // the second buffer is bigger
    appMemCacheWb(vec_interm_buffer_3, total_pixels * 2);
    appMemCacheWb(vec_dct_buff, total_pixels * 4);

    // Perform cycle calculation and print

    #ifdef DEBUG_CYCLE_COUNT
        // uint64_t diff_rgb   = y_time - start_time;
        // uint64_t diff_seg   = segmentation_time - y_time;
        // uint64_t diff_dct   = dct_time - segmentation_time;
        // uint64_t diff_quant = quantization_time - dct_time;
        // uint64_t diff_zz    = zig_zag_time - quantization_time;
        // uint64_t diff_enc   = encoding_time - zig_zag_time;
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
        
        // format_commas(diff_seg, fmt_buf);
        // offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "Segmentation (8x8)", fmt_buf);
        
        format_commas(total_dct_time, fmt_buf);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "DCT Transform (Total)", fmt_buf);

        // format_commas(timer1 - start, fmt_buf);
        // offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "  -> First matmul (C x f)", fmt_buf);
        
        // format_commas(timer2 - timer1, fmt_buf);
        // offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-42s | %22s |\n", "  -> Second matmul ((C x f) x C^T)", fmt_buf);
        
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


void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, int8_t *y_ptr, int num_pixels)
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
        y_temp = y_temp - val_128;              // center around zero because of DCTs

        vec_y[i] = convert_char32(y_temp);
    }
}
