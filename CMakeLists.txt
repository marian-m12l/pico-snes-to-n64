# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "/YOUR_PICO_SDK_PATH/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico-snes-n64 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

#set(PICO_CXX_ENABLE_EXCEPTIONS 1)

#set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Pico SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


# Add executable. Default name is the project name, version 0.1
add_executable(pico-snes-n64
                main.cpp
                joybus-pio/src/joybus.c
                joybus-pio/src/N64Console.cpp
             )

include_directories(joybus-pio/include)

pico_generate_pio_header(pico-snes-n64 ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/joybus.pio)

pico_generate_pio_header(pico-snes-n64 ${CMAKE_CURRENT_LIST_DIR}/snes.pio)
pico_set_program_name(pico-snes-n64 "pico-snes-n64")
pico_set_program_version(pico-snes-n64 "0.1")

pico_enable_stdio_uart(pico-snes-n64 1)
pico_enable_stdio_usb(pico-snes-n64 1)

# Add the standard library to the build
target_link_libraries(pico-snes-n64 pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico-snes-n64
                      hardware_timer
                      hardware_pio
                      hardware_dma
                      pico_multicore
                     )

pico_add_extra_outputs(pico-snes-n64)

