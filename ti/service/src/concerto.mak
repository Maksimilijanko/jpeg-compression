include $(PRELUDE)

TARGET      := app_utils_jpeg_compression
TARGETTYPE  := library

# Add your source file
CSOURCES    := jpeg_compression.c

# Include paths for your headers
IDIRS 		+= $(JPEG_COMPRESSION_PATH)/service/include
IDIRS 		+= $(JPEG_COMPRESSION_PATH)/client/include
IDIRS       += $(VISION_APPS_PATH)/utils/console_io/include
IDIRS       += $(VISION_APPS_PATH)/utils/remote_service/include
IDIRS       += $(VISION_APPS_PATH)/utils/ipc/include

include $(FINALE)
