add_library(bme280_driver STATIC
    src/bme280-driver.c
)

target_include_directories(bme280_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(bme280_driver
    pico_stdlib
    hardware_i2c
)