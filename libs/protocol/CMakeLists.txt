add_library(protocol STATIC
    src/protocol-task.c
)

target_include_directories(protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(protocol
    pico_stdlib
)