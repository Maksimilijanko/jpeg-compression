ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), C71))

include $(PRELUDE)

TARGET      := app_utils_jpeg_compression
TARGETTYPE  := library

# Add your source file
CSOURCES    := jpeg_compression.c quantization_table.c huffman_tables.c dct_matrix.c dct.c

# For passing through user flags
ifneq ($(USER_CFLAGS),)
    CFLAGS += $(USER_CFLAGS)
endif

# Include paths for your headers
IDIRS 		+= $(JPEG_COMPRESSION_PATH)/service/include
IDIRS       += $(VISION_APPS_PATH)/utils/console_io/include
IDIRS       += $(VISION_APPS_PATH)/utils/remote_service/include
IDIRS       += $(VISION_APPS_PATH)/utils/mem/include
IDIRS       += $(VISION_APPS_PATH)/utils/ipc/include
IDIRS       += $(PSDKR_PATH)/psdk_tools/ti-cgt-c7000_4.1.0.LTS/include

include $(FINALE)

endif