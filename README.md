# JPEG compression algorithm implementation in C

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

## 🛠️ Build & Setup Instructions

### 1. Prerequisites
You need the **TI Processor SDK RTOS (J721e)** version `09_02_00_05`.
* Download and install it on your Linux host machine.

### 2. SDK Configuration (One-Time Setup)
Before the first build, you must configure the paths and patch the SDK to include this custom application.
1.  **Export the Project Path:**
    The TI build system needs to know where this project is located. Run this in your terminal (from ti/ folder):
    ```bash
    export JPEG_COMPRESSION_PATH=$(pwd)
    ```

2.  Open the `setup_psdk.sh` script in a text editor.
3.  Update the `TI_SDK_PATH` variable to point to your installed SDK location:
    ```bash
    TI_SDK_PATH="/path/to/ti-processor-sdk-rtos-j721e-evm-09_02_00_05"
    ```
4.  Run the setup script:
    ```bash
    chmod +x setup_psdk.sh
    ./setup_psdk.sh
    ```
    *This script initializes the SDK git repository (for safety), configures build variables, and applies necessary patches.*

### 3. Building the Project
Use CMake to configure the project and trigger the TI Vision Apps build system.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
This will compile both the Host (PC) tools and the Target (C7x/A72) binaries.

### 4. Flashing to TDA4VM
To deploy the compiled binaries to the EVM board, use the provided flashing script. Ensure Ethernet connection is established with the board.

```bash
./flash_binaries.sh
```

---

## 💻 Usage (On Target)

Once the board is booted, run the compiled executable from the terminal.

```bash
cd /opt/vision_apps
./app_jpeg_compression.out
```

*Note: Ensure input files (if any) are placed in the correct directory as expected by the application.*

---

## 💻 Usage (PC / Host Simulation)

You can also run the C-model encoder/decoder locally on your PC:

```bash
./jpeg_encoder -input input.bmp -output output.jpeg
```

**Note:** The input image must be a valid 24-bit BMP file.


## 📂 Project Structure

```text
.
├── assets
│   ├── input                           # Source BMP images for testing
│   │   ├── grad.bmp
│   │   └── lena.bmp
│   └── output                          # Generated JPEG files go here
├── natural_c                           # Pure C implementation (Host/PC)
│   ├── CMakeLists.txt                  # Build config for PC executable
│   ├── include                         # Algorithm header files
│   │   ├── bmp_handler.h               # BMP file parsing headers
│   │   ├── color_spaces.h              # RGB <-> YCbCr conversion headers
│   │   ├── dct.h                       # Discrete Cosine Transform headers
│   │   ├── grayscale.h                 # Grayscale conversion headers
│   │   └── jfif_handler.h              # JPEG file structure headers
│   └── src                             # Algorithm source implementation
│       ├── bmp_handler.c               # BMP reading/writing logic
│       ├── color_spaces.c              # Color space conversion logic
│       ├── dct.c                       # 8x8 Block DCT implementation
│       ├── grayscale.c                 # Simple grayscale conversion logic
│       ├── huffman_tables.c            # Standard JPEG Huffman tables
│       ├── jfif_handler.c              # JPEG bitstream construction
│       ├── main.c                      # Entry point for PC application
│       └── quantization_table.c        # Standard JPEG Quantization tables
├── ti                                  # TI TDA4VM specific implementation (Target)
│   ├── client                          # A72 (Linux) Host application
│   │   ├── concerto.mak                # Build config for A72 core
│   │   └── main.c                      # Client app
│   ├── src                             # C7x (DSP) Kernel implementation
│   │   ├── concerto.mak                # Build config for C7x core
│   │   └── jpeg_compression.c          # DSP-optimized kernel logic
│   ├── include                         # Shared headers for TI components
│   │   └── jpeg_compression.h          # Kernel interface definition
│   ├── CMakeLists.txt                  # Wrapper to trigger TI build system
│   ├── flash_binaries.sh               # Helper script to deploy to SD card
│   ├── jpeg_compression_ti_psdk.patch  # Git patch for SDK integration
│   └── setup_psdk.sh                   # Script to configure paths & apply patch
├── CMakeLists.txt                      # Root CMake configuration
├── LICENSE
└── README.md
```

## 🔮 Future Improvements

* Algorithm optimization for DSP execution
* Image decompression
