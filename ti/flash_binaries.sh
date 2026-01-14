#!/bin/sh

if [ -z "$TI_PSDK_PATH" ]; then
    echo "Error: Environment variable 'TI_PSDK_PATH' not defined."
    echo "Run the following command: export TI_PSDK_PATH=/path/to/your/psdk"
    exit 1
fi

# ===== Configuration =====
TARGET_USER=root
TARGET_IP=192.168.1.200
TARGET_DIR=/lib/firmware/vision_apps_evm
APPS_TARGET_DIR=/opt/vision_apps

BINARIES="
$TI_PSDK_PATH/vision_apps/out/J721E/R5F/FREERTOS/release/*.out
$TI_PSDK_PATH/vision_apps/out/J721E/C71/FREERTOS/release/*.out
$TI_PSDK_PATH/vision_apps/out/J721E/C66/FREERTOS/release/*.out
"

APPS="
$TI_PSDK_PATH/vision_apps/out/J721E/A72/LINUX/release/app_jpeg_compression.out
$TI_PSDK_PATH/vision_apps/out/J721E/A72/LINUX/release/app_jpeg_compression.out.map"

# ===== Copy binaries =====
echo "Copying RTOS binaries to target..."

for BIN in $BINARIES; do
    echo "  -> $BIN"
    scp "$BIN" ${TARGET_USER}@${TARGET_IP}:${TARGET_DIR}/ || {
        echo "ERROR: Failed to copy $BIN"
        exit 1
    }
done

echo "Copying apps binaries to target..."
scp $APPS ${TARGET_USER}@${TARGET_IP}:${APPS_TARGET_DIR}

# ===== Restart target =====
# echo "Rebooting target..."
# ssh ${TARGET_USER}@${TARGET_IP} "sync && reboot"

echo "Done."