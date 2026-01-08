#include <stdio.h>
#include <string.h>
#include <TI/tivx.h>
#include <utils/ipc/include/app_ipc.h>
#include <utils/remote_service/include/app_remote_service.h>
#include <utils/app_init/include/app_init.h>
#include <utils/console_io/include/app_log.h>
#include "jpeg_compression.h"

#include "bmp_handler.h"
#include "color_spaces.h"

int main(int argc, char **argv)
{
    PARAMETERS params = parse_parameters(argc, argv);

    BMP_IMAGE image = load_bmp_image(params.inputFile); 

    // pixels is dyn. allocated - needs to be freed
    RGB* pixels = read_pixels(image.buffer, image.info.width, image.info.height, image.info.height <= 0);

    printf("BMP image imported.\n");
    printf("Original size: %u x %u\n", image.info.width, image.info.height);
    printf("File size: %u\n", image.header.file_size);

    printf("Zero pixel B component: %f\n", pixels[0].b);

    return 0;
}
