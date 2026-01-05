# JPEG compression algorithm optimized for DSP execution

A lightweight, dependency-free JPEG encoder implementation written in pure C. 
This project demonstrates the inner workings of the JPEG compression algorithm, converting BMP images into valid `.jpg` files without using external libraries like `libjpeg` or `stb_image`.

## 🚀 Features

* **BMP Support:** Parses standard BMP files (handles bottom-up pixel storage).
* **Grayscale Encoding:** Converts RGB input to Luminance (Y) channel.
* **JPEG Pipeline Implementation:**
    * Color space conversion (RGB $\to$ Y).
    * 8x8 block splitting.
    * Discrete Cosine Transform (DCT).
    * Quantization (using standard Luminance tables).
    * ZigZag reordering.
    * Predictive and RLE encoding.
    * Huffman entropy encoding.

## 🛠️ Build Instructions

This project uses **CMake** for building. Ensure you have a C compiler (GCC/Clang) and CMake installed.

### Linux / macOS

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Generate Makefiles
cmake ..

# 3. Compile the project
make
```

## 💻 Usage

Run the compiled executable from the command line, providing the input BMP file and the desired output filename.

```bash
./jpeg_encoder -input input.bmp -output output.jpeg
```

**Note:** The input image must be a valid 24-bit BMP file.

## 📂 Project Structure

```text
.
├── assets
│   ├── input                   # Source BMP images for testing
│   │   ├── grad.bmp
│   │   ├── lena.bmp
│   │   └── sample_1280x853.bmp
│   └── output                  # Generated JPEG files go here
├── CMakeLists.txt              # Build system configuration
├── include                     # Header files (.h)
│   ├── bmp_handler.h
│   ├── color_spaces.h
│   ├── dct.h
│   ├── grayscale.h
│   └── jfif_handler.h
├── LICENSE
├── README.md
└── src                         # Source implementation (.c)
    ├── bmp_handler.c           # BMP parsing logic
    ├── color_spaces.c          # Pixel data reading and color space switching functions
    ├── dct.c                   # Discrete Cosine Transform and encoding 
    ├── grayscale.c             # RGB to Luminance conversion
    ├── huffman_tables.c        # Standard Huffman tables
    ├── jfif_handler.c          # JPEG bitstream writer
    ├── main.c                  # Entry point
    └── quantization_table.c    # Standard JPEG quantization table
    