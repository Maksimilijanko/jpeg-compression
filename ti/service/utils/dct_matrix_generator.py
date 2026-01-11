import math
import sys

def generate_dct_matrices(filename):
    N = 8
    C = [[0.0] * N for _ in range(N)]
    C_T = [[0.0] * N for _ in range(N)]

    inv_sqrt_N = 1.0 / math.sqrt(N)
    sqrt_2_div_N = math.sqrt(2.0 / N)

    # Calculate coefficients
    for u in range(N):
        if u == 0:
            alpha = inv_sqrt_N
        else:
            alpha = sqrt_2_div_N
            
        for x in range(N):
            angle = (math.pi / N) * (x + 0.5) * u
            val = alpha * math.cos(angle)
            C[u][x] = val
            C_T[x][u] = val

    try:
        with open(filename, 'w') as f:
            # Header
            f.write("/* Generated DCT 8x8 Coefficients (Type II) */\n")
            f.write("/* Matrices will be placed onto adresses divisible by 64B in order to avoid false addresses, enhance efficienty of .D units, as well as Streaming Engine */\n")
            f.write("#include <stdint.h>\n\n")

            # Standard Matrix C
            f.write("// DCT Matrix C (row-major)\n")
            f.write("const float dct_matrix_c[64] __attribute__((aligned(64))) = {\n")
            for row in C:
                f.write("    ")
                for val in row:
                    f.write(f"{val:10.7f}f, ")
                f.write("\n")
            f.write("};\n\n")

            # Transposed Matrix C^T
            f.write("// Transposed DCT Matrix C^T (row-major)\n")
            f.write("const float dct_matrix_c_T[64] __attribute__((aligned(64))) = {\n")
            for row in C_T:
                f.write("    ")
                for val in row:
                    f.write(f"{val:10.7f}f, ")
                f.write("\n")
            f.write("};\n")
            
        print(f"[SUCCESS] DCT matrices written to: {filename}")

    except IOError as e:
        print(f"[ERROR] Could not write to file: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python gen_dct.py <output_filename.c>")
    else:
        generate_dct_matrices(sys.argv[1])
