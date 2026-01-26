import cv2
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from skimage.metrics import structural_similarity as ssim
import argparse
import sys
import os

def get_file_size_kb(path):
    """Returns file size in KB."""
    return os.path.getsize(path) / 1024.0

def calculate_metrics(img1, img2):
    """
    Calculates MSE, PSNR, and SSIM.
    """
    i1 = img1.astype(np.float64)
    i2 = img2.astype(np.float64)
    
    mse = np.mean((i1 - i2) ** 2)
    
    if mse == 0:
        psnr = 100 # Infinite quality (perfect match)
    else:
        max_pixel = 255.0
        psnr = 20 * np.log10(max_pixel / np.sqrt(mse))
        
    # SSIM (Structural Similarity Index)
    ssim_val = ssim(img1, img2, data_range=255)
    
    return mse, psnr, ssim_val

if __name__ == "__main__":
    # 1. Argument Parsing
    parser = argparse.ArgumentParser(description="Advanced Image Analysis (MSE, PSNR, SSIM)")
    parser.add_argument("-r", "--ref", required=True, help="Path to Original (Reference) image")
    parser.add_argument("-j", "--jpeg", required=True, help="Path to Compressed (JPEG) image")
    args = parser.parse_args()

    # 2. Validation
    if not os.path.exists(args.ref) or not os.path.exists(args.jpeg):
        print("Error: Input files not found.")
        sys.exit(1)

    # 3. Load Images
    print("Loading images...")
    img_orig = cv2.imread(args.ref, cv2.IMREAD_GRAYSCALE)
    img_comp = cv2.imread(args.jpeg, cv2.IMREAD_GRAYSCALE)

    if img_orig is None or img_comp is None:
        print("Error: Could not decode images.")
        sys.exit(1)

    # 4. Dimension Handling
    h_min = min(img_orig.shape[0], img_comp.shape[0])
    w_min = min(img_orig.shape[1], img_comp.shape[1])
    
    # Crop to match sizes if necessary
    img_orig = img_orig[:h_min, :w_min]
    img_comp = img_comp[:h_min, :w_min]
    
    dims_text = f"{w_min}x{h_min}"

    # 5. Calculation
    print("Calculating metrics...")
    mse_val, psnr_val, ssim_val = calculate_metrics(img_orig, img_comp)
    
    # File sizes
    size_orig = get_file_size_kb(args.ref)
    size_comp = get_file_size_kb(args.jpeg)
    
    compression_ratio = size_orig / size_comp if size_comp > 0 else 0
    space_saving = (1 - (size_comp / size_orig)) * 100

    # --- VISUALIZATION ---
    
    # Set a large figure size (Width, Height)
    fig = plt.figure(figsize=(18, 10))
    plt.suptitle(f"Compression Analysis: {os.path.basename(args.jpeg)}", fontsize=16, fontweight='bold')

    # Use GridSpec to control layout ratios
    # height_ratios=[4, 1] means images get 4 parts height, text gets 1 part
    gs = GridSpec(2, 3, figure=fig, height_ratios=[4, 1], wspace=0.1, hspace=0.15)

    # --- PLOT 1: ORIGINAL ---
    ax1 = fig.add_subplot(gs[0, 0])
    ax1.imshow(img_orig, cmap='gray', vmin=0, vmax=255)
    ax1.set_title(f"ORIGINAL ({dims_text})\nSize: {size_orig:.2f} KB", fontsize=12, fontweight='bold', color='#333333')
    ax1.axis('off')

    # --- PLOT 2: COMPRESSED ---
    ax2 = fig.add_subplot(gs[0, 1])
    ax2.imshow(img_comp, cmap='gray', vmin=0, vmax=255)
    ax2.set_title(f"COMPRESSED ({dims_text})\nSize: {size_comp:.2f} KB", fontsize=12, fontweight='bold', color='#333333')
    ax2.axis('off')

    # --- PLOT 3: DIFFERENCE ---
    ax3 = fig.add_subplot(gs[0, 2])
    diff = cv2.absdiff(img_orig, img_comp)
    # Amplified x5 for visibility
    im_diff = ax3.imshow(diff * 5, cmap='inferno') 
    ax3.set_title("DIFFERENCE MAP\n(Amplified x5)", fontsize=12, fontweight='bold', color='#333333')
    ax3.axis('off')
    
    # Colorbar for difference (small and separate)
    cbar = plt.colorbar(im_diff, ax=ax3, fraction=0.046, pad=0.04)
    cbar.ax.tick_params(labelsize=8)

    # --- TEXT METRICS (Bottom Row, spanning all columns) ---
    ax_text = fig.add_subplot(gs[1, :])
    ax_text.axis('off') # Hide axis for text area

    stats_text = (
        f" QUALITY METRICS                        COMPRESSION STATISTICS\n"
        f" ------------------------------------   ------------------------------------\n"
        f" MSE  (Mean Sq. Error) : {mse_val:8.4f}       Original Size     : {size_orig:8.2f} KB\n"
        f" PSNR (Signal-to-Noise): {psnr_val:8.4f} dB     Compressed Size   : {size_comp:8.2f} KB\n"
        f" SSIM (Similarity)     : {ssim_val:8.4f}       Compression Ratio : {compression_ratio:8.2f} : 1\n"
        f"                                        Space Saving      : {space_saving:8.2f} %"
    )

    # Add text box centered in the bottom area
    ax_text.text(0.5, 0.5, stats_text, 
                 fontsize=13, family='monospace', 
                 ha="center", va="center",
                 bbox={"facecolor":"#f4f4f4", "edgecolor":"#dddddd", "boxstyle":"round,pad=1"})

    print("Displaying results...")
    plt.show()