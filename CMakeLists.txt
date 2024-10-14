# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(hog_keyboard C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# usb_hid_host

add_library(usb_hid_host)
target_sources(usb_hid_host PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    )
target_include_directories(usb_hid_host PRIVATE   
    ${CMAKE_CURRENT_LIST_DIR} # For tusb config
    )
target_link_libraries(usb_hid_host PRIVATE
    pico_stdlib 
    tinyusb_host 
    tinyusb_board
    )
# add_compile_definitions(
#     CFG_TUSB_MCU=OPT_MCU_RP2040
#     CFG_TUSB_OS=OPT_OS_PICO
# )

# hog_keyboard

# Add executable. Default name is the project name, version 0.1
add_executable(hog_keyboard
    hog_keyboard.c
    picow_bt_example_background.c
    # picow_bt_example_common.c
    )
target_link_libraries(hog_keyboard
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    # usb_hid_host
    )

target_include_directories(hog_keyboard PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    ${PICO_SDK_PATH}/lib/tinyusb/src/class/hid
    ${PICO_SDK_PATH}/lib/tinyusb/src
    )
target_compile_definitions(hog_keyboard PRIVATE
    HAVE_BTSTACK_STDIN=1
    )
pico_btstack_make_gatt_header(hog_keyboard PRIVATE "${CMAKE_CURRENT_LIST_DIR}/hog_keyboard.gatt")

pico_add_extra_outputs(hog_keyboard)

pico_enable_stdio_usb(hog_keyboard 0)
pico_enable_stdio_uart(hog_keyboard 1)

