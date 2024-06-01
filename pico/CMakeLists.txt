cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ../../../pico-sdk)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(pico
  main.c)

pico_enable_stdio_usb(pico 1)
pico_enable_stdio_uart(pico 0)
pico_add_extra_outputs(pico)

target_link_libraries(pico pico_stdlib hardware_adc)
