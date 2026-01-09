$(warning [DEBUG] concerto.mak file in ti/client/src/concerto.mak is found!)

ifeq ($(TARGET_CPU),$(filter $(TARGET_CPU), A72))
ifeq ($(TARGET_OS),$(filter $(TARGET_OS), LINUX QNX))

include $(PRELUDE)

# Source files
CSOURCES    := main.c bmp_handler.c color_spaces.c

# Name of the output executable (.out)
TARGET      := app_jpeg_compression
TARGETTYPE  := exe

include $(VISION_APPS_PATH)/apps/concerto_mpu_inc.mak

# Include path for shared headers
IDIRS       += $(JPEG_COMPRESSION_PATH)/service/include

# Include path for client-specific headers
IDIRS       += $(JPEG_COMPRESSION_PATH)/client/include

include $(FINALE)

endif
endif