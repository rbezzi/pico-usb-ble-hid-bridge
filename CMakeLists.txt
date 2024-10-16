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

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(usb_ble_hid_bridge C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# usb_hid_host

add_library(usb_hid_host)
target_sources(usb_hid_host PUBLIC
    tusb_hid.c
    )
target_include_directories(usb_hid_host PUBLIC   
    .
    ${PICO_SDK_PATH}/lib/tinyusb/src/class/hid
    ${PICO_SDK_PATH}/lib/tinyusb/src
    )
target_link_libraries(usb_hid_host PUBLIC
    pico_stdlib 
    tinyusb_host 
    tinyusb_board
    )

# hog_keyboard

add_library(hog_keyboard)
target_sources(hog_keyboard PUBLIC
    hog_keyboard.c
    )
target_include_directories(hog_keyboard PUBLIC
    .
    )
target_link_libraries(hog_keyboard PUBLIC
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    )
target_compile_definitions(hog_keyboard PUBLIC
    HAVE_BTSTACK_STDIN=1
    )
pico_btstack_make_gatt_header(hog_keyboard PUBLIC "${CMAKE_CURRENT_LIST_DIR}/hog_keyboard.gatt")


add_executable(usb_ble_hid_bridge
    main.c
)
target_include_directories(usb_ble_hid_bridge PUBLIC
    .
)
target_link_libraries(usb_ble_hid_bridge
    hog_keyboard
    usb_hid_host
)

pico_add_extra_outputs(usb_ble_hid_bridge)

pico_enable_stdio_usb(usb_ble_hid_bridge 0)
pico_enable_stdio_uart(usb_ble_hid_bridge 1)

