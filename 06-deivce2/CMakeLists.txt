cmake_minimum_required(VERSION 3.15)

# Указываем путь к Pico SDK (замените путь на ваш, если нужно)
set(PICO_SDK_PATH "C:/Users/Vika/Desktop/Repo/Pico/pico-sdk")

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(06-device)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/protocol ${CMAKE_CURRENT_BINARY_DIR}/protocol)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/bme280-driver ${CMAKE_CURRENT_BINARY_DIR}/bme280)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/ili9341-driver ${CMAKE_CURRENT_BINARY_DIR}/ili9341-driver)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-task/stdio-task.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    hardware_spi
    protocol
    bme280_driver
    ili9341-driver
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/protocol/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/bme280-driver/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/ili9341-driver/include
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-task
)

pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})