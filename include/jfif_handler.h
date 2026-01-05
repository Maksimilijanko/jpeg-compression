#include <stdio.h>
#include <stdint.h>

/*
* JFIF Handler
* Contains function definitions for handling JFIF serialization.
*/

/* 
* Standard Luminance quantization table.
*/
extern const uint8_t std_lum_qt[64];

/*
* Writes 2 bytes into file.
* Input: pointer to a open file
* Input: two-byte data
*/
void write_word(FILE *f, uint16_t v);

/*
* Writes SOI marker - Start of Image
*/
void write_soi(FILE *f);

/*
* Writes APP0 marker
*/
void write_app0(FILE *f);

/*
* Writes DQT marker - Define Quantization Table
* It uses external quantization table std_lum_qt
*/
void write_dqt(FILE *f);

/*
* Writes SOF0 marker - Start of Frame
* Input: width of the picture
* Input: height of the picture
*/
void write_sof0(FILE *f, uint16_t width, uint16_t height);

/*
* Writes DHT marker - Define Huffman Table
*/
void write_dht(FILE *f);

/*
* Writes SOS marker - Start of Scan
*/
void write_sos(FILE *f);

/*
* Writes EOI marker - End of Image
*/
void write_eoi(FILE *f);

/* 
* Write processed image bytes.
*/
void write_bitstream(FILE *f, uint8_t *buffer, int length);

/*
* Perform image serialization into a JFIF file.
* Input: file to write to
* Input: buffer containing processed bytes of image
* Input: buffer length
* Input: image width
* Input: image height
*/
void write_to_jfif(FILE *f, uint8_t *buffer, int length, uint16_t width, uint16_t height);