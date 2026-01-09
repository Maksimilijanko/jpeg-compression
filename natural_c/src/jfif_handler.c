#include <stdio.h>
#include <stdint.h>
#include "jfif_handler.h"

void write_word(FILE *f, uint16_t v) {
    fputc((v >> 8) & 0xFF, f);
    fputc(v & 0xFF, f);
}

void write_soi(FILE *f) {
    fputc(0xFF, f); 
    fputc(0xD8, f);     // SOI marker
}

void write_app0(FILE *f) {
    fputc(0xFF, f);
    fputc(0xEE, f);

    write_word(f, 16);          // length of this segment (16 for standard JFIF)

    fputc('J', f);
    fputc('F', f);
    fputc('I', f);
    fputc('F', f);
    fputc(0x00, f);         // Write 'JFIF' ending with 0x00

    fputc(0x01, f);
    fputc(0x01, f);         // version: 1.01

    fputc(0x00, f);         // ones (0 = none)

    write_word(f, 1);       // X density (1 pixel aspect ratio)
    write_word(f, 1);       // Y density (1 pixel aspect ratio)

    fputc(0x00, f);         // thumbnail width (0 = no thumbnail)
    fputc(0x00, f);         // thumbnail height (0 = no thumbnail)
}

void write_dqt(FILE *f) {
    fputc(0xFF, f);
    fputc(0xDB, f);         // DQT marker

    write_word(f, 67);      // Length: 2 bytes length data + 1 byte info + 64 bytes quantization table

    fputc(0x00, f);         // info byte: 
                            // upper 4 bits represent precision (0 = 8-bit)
                            // lower 4 bits represent table ID (0 = Luminance)

    fwrite(std_lum_qt_zigzagged, 1, 64, f);
}

void write_sof0(FILE *f, uint16_t width, uint16_t height) {
    fputc(0xFF, f);
    fputc(0xC0, f); // SOF0 marker
    
    write_word(f, 11);      // length: 8 + 3 * number_of_components (grayscale, so there is only one component)
    
    fputc(8, f);            // Precision: 8 bits per sample
    write_word(f, height);  // picture height 
    write_word(f, width);   // picture width
    fputc(1, f);            // number of componenets (1 = Grayscale)
    
    // For each component (there is only 1)
    fputc(1, f);    // Component ID (1 = Y / Luminance)
    fputc(0x11, f); // Sampling factors. 1x1 is the standard value.
    fputc(0, f);    // Quantization Table ID (0 for the table we used)
}

void write_dht(FILE *f) {
    // --- DC Table (Luminance) ---
    uint8_t dc_lum_bits[16] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    // symbols sorted by frequency
    uint8_t dc_lum_vals[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    
    fputc(0xFF, f);
    fputc(0xC4, f);     // DHT marker
    
    // Segment length: 2 (length bytes) + 1 (info) + 16 (bits) + 12 (vals) = 31
    write_word(f, 2 + 1 + 16 + sizeof(dc_lum_vals));
    
    // Info byte:
    // 4th bit: class (0 = DC, 1 = AC)
    // Bits 0-3: table ID (0 = Luminance)
    fputc(0x00, f); // 00 = DC Table 0
    
    fwrite(dc_lum_bits, 1, 16, f);
    fwrite(dc_lum_vals, 1, sizeof(dc_lum_vals), f);
    
    // --- AC table (Luminance) ---
    uint8_t ac_lum_bits[16] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7D};
    uint8_t ac_lum_vals[] = { 
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
        0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
        0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
        0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
        0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA
    };
    
    fputc(0xFF, f);
    fputc(0xC4, f); // DHT marker
    
    // Segment length
    write_word(f, 2 + 1 + 16 + sizeof(ac_lum_vals));
    
    // Info byte:
    // 4th bit: class (1 = AC)
    // Bits 0-3: table ID (0 = Luminance)
    fputc(0x10, f); // 10 = AC Table 0
    
    fwrite(ac_lum_bits, 1, 16, f);
    fwrite(ac_lum_vals, 1, sizeof(ac_lum_vals), f);
}

void write_sos(FILE *f) {
    fputc(0xFF, f);
    fputc(0xDA, f); // SOS marker
    
    // Length: 6 + 2 * number_of_components
    // Grayscale: 6 + 2 = 8
    write_word(f, 8);
    
    fputc(1, f); // Num of components in this scan
    
    // Component 1 (Y)
    fputc(1, f); // Component ID
    // Defines which Huffman table to use
    // Upper 4 bits: DC table ID (0)
    // Lower 4 bits: AC table ID (0)
    fputc(0x00, f); 
    
    // 3 bytes for spectral selection (Baseline standard):
    fputc(0x00, f); // Start of spectral selection
    fputc(0x3F, f); // End of spectral selection (63)
    fputc(0x00, f); // Successive approximation
}

void write_eoi(FILE *f) {
    fputc(0xFF, f);
    fputc(0xD9, f);     // EOI marker
}

void write_bitstream(FILE *f, uint8_t *buffer, int length) {
    fwrite(buffer, 1, length, f);
}

void write_to_jfif(FILE *f, uint8_t *buffer, int length, uint16_t width, uint16_t height) {
    write_soi(f);
    write_app0(f);
    write_dqt(f);      
    write_sof0(f, width, height);
    write_dht(f);      
    write_sos(f);

    // --- processed data --- 
    write_bitstream(f, buffer, length);

    write_eoi(f);

}
