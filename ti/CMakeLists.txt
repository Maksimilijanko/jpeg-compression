cmake_minimum_required(VERSION 3.15)

project(JPEG_Compression_TI LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{TI_PSDK_PATH})
    message(FATAL_ERROR "Error: Environment variable 'TI_PSDK_PATH' not defined!\n"
            "Run the following command: export TI_PSDK_PATH=/path/to/your/psdk")
endif()

if(NOT DEFINED ENV{CGT7X_ROOT})
    message(FATAL_ERROR "Error: Environment variable 'CGT7X_ROOT' not defined!\n"
            "Run the following command: export CGT7X_ROOT=/path/to/your/cgt7x/root")
endif()


# -----------------------------------------------------------------------------
# MAIN BUILD TARGET
# -----------------------------------------------------------------------------

# TI PDSK path (sdk_builder)
set(TI_PSDK_PATH "$ENV{TI_PSDK_PATH}/sdk_builder")

# Root PSDK path
set(ROOT_SDK_ROOT "$ENV{TI_PSDK_PATH}")

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

# -----------------------------------------------------------------------------
# DEBUG BUILD (Profiling + Assembly Analysis)
# -----------------------------------------------------------------------------

set(CGT7X_ROOT      "$ENV{CGT7X_ROOT}")
set(VISION_APPS     "${ROOT_SDK_ROOT}/vision_apps")
set(TIOVX_PATH      "${ROOT_SDK_ROOT}/tiovx")
set(APP_UTILS_PATH  "${ROOT_SDK_ROOT}/app_utils")
set(CL7X_COMPILER   "${CGT7X_ROOT}/bin/cl7x")

set(DEBUG_OUT_DIR   "${CMAKE_CURRENT_BINARY_DIR}/ti_debug_analysis")

set(C7X_DEBUG_FLAGS "-O3")
list(APPEND C7X_DEBUG_FLAGS "--gen_opt_info=2")
list(APPEND C7X_DEBUG_FLAGS "-k")
list(APPEND C7X_DEBUG_FLAGS "--src_interlist")
list(APPEND C7X_DEBUG_FLAGS "--silicon_version=7100")
list(APPEND C7X_DEBUG_FLAGS "-DSOC_J721E")
list(APPEND C7X_DEBUG_FLAGS "-DTARGET_C71")
list(APPEND C7X_DEBUG_FLAGS "-DDEBUG_CYCLE_COUNT")          # custom flag for enabling c7x profiling code

set(C7X_DEBUG_INCLUDES
    -I "${CGT7X_ROOT}/include"
    -I "${APP_UTILS_PATH}"
    -I "${VISION_APPS}"
    -I "${TIOVX_PATH}/include"
    -I "${TIOVX_PATH}/kernels/include"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/service/include"                    # Service include dir
)

file(GLOB JPEG_DEBUG_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/service/src/*.c")

if(NOT JPEG_DEBUG_SRCS)
    message(WARNING "Debug Target: .c files not found in ${CMAKE_CURRENT_SOURCE_DIR}/service/src/")
endif()

string(REPLACE ";" " " C7X_FLAGS_STRING "${C7X_DEBUG_FLAGS}")

add_custom_target(trigger_ti_debug_build
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEBUG_OUT_DIR}
    COMMENT "--- Start C7x Profiling Build: Output in ${DEBUG_OUT_DIR} ---"

    COMMAND ${MAKE_EXE} -C ${TI_PSDK_PATH} 
            JPEG_COMPRESSION_PATH=${CMAKE_CURRENT_SOURCE_DIR} 
            SOC=j721e
            vision_apps
            USER_CFLAGS=${C7X_FLAGS_STRING}
    
    USES_TERMINAL
    
    COMMENT ">>> Running external TI PSDK Build..."
)

foreach(SRC_FILE ${JPEG_DEBUG_SRCS})
    get_filename_component(F_NAME ${SRC_FILE} NAME)
    
    add_custom_command(TARGET trigger_ti_debug_build
        POST_BUILD
        COMMAND ${CL7X_COMPILER} ${C7X_DEBUG_FLAGS} ${C7X_DEBUG_INCLUDES}
                -fr="${DEBUG_OUT_DIR}" 
                -fs="${DEBUG_OUT_DIR}" 
                "${SRC_FILE}"
        COMMENT "Analysing: ${F_NAME} -> .asm generated"
        WORKING_DIRECTORY ${DEBUG_OUT_DIR}
    )
endforeach()

add_custom_command(TARGET trigger_ti_debug_build
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "--- C7x Analysis Complete. Check .asm files in ${DEBUG_OUT_DIR} ---"
)