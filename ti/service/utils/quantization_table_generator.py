import os

# Standard Luminance Quantization Table
STD_LUM_QT = [
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
]

OUTPUT_FILENAME = "../src/quantization_table_reciprocal.c"
ARRAY_NAME = "std_lum_qt_recip"

def generate_file():
    print(f"Generating file: {OUTPUT_FILENAME}...")
    
    with open(OUTPUT_FILENAME, "w") as f:
        f.write("/*\n")
        f.write(" * Reciprocal value of quantization table (1.0/val).\n")
        f.write(" */\n\n")
        
        f.write("#include <stdint.h>\n\n")
        
        f.write(f"const float {ARRAY_NAME}[64] = {{\n")
        
        for i, val in enumerate(STD_LUM_QT):
            if val == 0:
                recip = 0.0
            else:
                recip = 1.0 / val
            
            entry = f"{recip:.9f}f"
            
            if i < len(STD_LUM_QT) - 1:
                entry += ","
            
            if i % 8 == 0:
                f.write("    ")
            
            f.write(f"{entry:<15}")
            
            if (i + 1) % 8 == 0:
                f.write("\n")
        
        f.write("};\n")
        
    print("Done.")

if __name__ == "__main__":
    generate_file()