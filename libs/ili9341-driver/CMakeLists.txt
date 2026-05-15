cmake_minimum_required(VERSION 3.15)


add_library(ili9341-driver STATIC
    src/ili9341-driver.c
    src/ili9341-display.c
    src/ili9341-font.c
    src/font-jetbrains.c
)

target_include_directories(ili9341-driver PUBLIC
    include
)

target_link_libraries(ili9341-driver
    pico_stdlib
    hardware_spi
)