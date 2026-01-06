#!/bin/bash

# ==============================================================================
# CONFIGURATION
# ==============================================================================
# Path to your TI RTOS SDK root directory
# TODO: Update this path if it matches your specific installation
TI_SDK_PATH="$HOME/ti-processor-sdk-rtos-j721e-evm-09_02_00_05"

# Name of the patch file (must be located in the same directory as this script)
PATCH_FILE="jpeg_compression_ti_psdk.patch"

# Store current project root to locate the patch file later
PROJECT_ROOT="$PWD"

# ==============================================================================
# STEP 1: FIRST-TIME SETUP PSDK
# ==============================================================================

echo "--------------------------------------------------------"
echo ">>> STEP 1: First-time TI PSDK setup..."

cd "$TI_SDK_PATH"
# Run setup
./sdk_builder/scripts/setup_psdk_rtos.sh

cd "$PROJECT_ROOT"

# ==============================================================================
# STEP 2: PREPARE VISION APPS DIRECTORY (GIT INIT)
# ==============================================================================
echo "--------------------------------------------------------"
echo ">>> STEP 2: Preparing Vision Apps directory..."

VISION_APPS_DIR="$TI_SDK_PATH/vision_apps"

# 1. Check if the SDK directory exists
if [ ! -d "$VISION_APPS_DIR" ]; then
    echo "ERROR: Directory not found: $VISION_APPS_DIR"
    echo "Please update the 'TI_SDK_PATH' variable in this script."
    exit 1
fi

# Navigate to the SDK folder
cd "$VISION_APPS_DIR"

# 2. Check if it is already a git repository
if [ ! -d ".git" ]; then
    echo "INFO: Vision Apps is not a git repository. Initializing..."
    
    # Initialize git repo to track changes safely
    git init
    
    # Stage all factory files
    git add .
    
    # Create an initial commit (Snapshot of the clean SDK)
    git commit -m "Clean Factory SDK (Before Custom Patching)" --quiet
    
    echo "SUCCESS: Git repository created. You now have a safety rollback point."
else
    echo "INFO: Vision Apps is already a git repository. Proceeding..."
fi

# ==============================================================================
# STEP 2: APPLY CUSTOM PATCH
# ==============================================================================
echo "--------------------------------------------------------"
echo ">>> STEP 2: Applying JPEG Integration Patch..."

FULL_PATCH_PATH="$PROJECT_ROOT/$PATCH_FILE"

# 1. Verify patch file existence
if [ ! -f "$FULL_PATCH_PATH" ]; then
    echo "ERROR: Patch file not found at: $FULL_PATCH_PATH"
    exit 1
fi

# 2. Check if the patch can be applied without errors (Dry Run)
# We stifle stderr because git apply --check prints errors if it fails
if git apply --check "$FULL_PATCH_PATH" 2>/dev/null; then
    
    # If check passes, apply the patch for real
    git apply --whitespace=nowarn "$FULL_PATCH_PATH"
    echo "SUCCESS: Patch applied successfully!"

else
    # If check fails, we need to know WHY.
    # Usually it's because the patch is already applied.
    
    # We check for a specific signature string from your code changes
    if grep -q "JpegCompression_Init" platform/j721e/rtos/common/app_init.c; then
        echo "INFO: Patch seems to be already applied. No changes made."
    else
        # If the code isn't there but git apply failed, it's a genuine conflict
        echo "ERROR: Patch failed to apply due to conflicts."
        echo "SUGGESTION: Since this is a git repo, you can reset the SDK using:"
        echo "  cd $VISION_APPS_DIR && git reset --hard"
        exit 1
    fi
fi

echo "--------------------------------------------------------"
echo "SETUP COMPLETED. The SDK is ready for building."
echo "--------------------------------------------------------"