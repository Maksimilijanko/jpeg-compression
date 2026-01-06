cmake_minimum_required(VERSION 3.15)

project(JPEG_Compression_TI LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# TI PDSK path
set(TI_PSDK_PATH $ENV{HOME}/ti-processor-sdk-rtos-j721e-evm-09_02_00_05/sdk_builder)

find_program(MAKE_EXE NAMES make gmake REQUIRED)

# Create a target to trigger TI build
add_custom_target(trigger_ti_build
    COMMAND ${MAKE_EXE} -C ${TI_PSDK_PATH} 
            JPEG_COMPRESSION_PATH=${CMAKE_CURRENT_SOURCE_DIR} 
            SOC=j721e
            vision_apps
    
    USES_TERMINAL
    
    COMMENT ">>> Running external TI PSDK Build..."
)

# # 4. (OPCIONO) Ako imaš lokalne stvari koje se moraju desiti PRIJE TI builda
# # Recimo da ovdje lokalno generišeš neki header ili biblioteku
# add_custom_target(moje_lokalne_pripreme
#     COMMAND echo "Radim lokalnu pripremu..."
#     # COMMAND python3 scripts/generate_config.py
# )

# # 5. Uvezivanje: Kažemo da TI build zavisi od lokalne pripreme
# add_dependencies(trigger_ti_build moje_lokalne_pripreme)