cmake_minimum_required(VERSION 3.10)

add_library(pn532_lib
    pn532.c
    pn532_rpi.c
    rpi_tcptest/pn532_wrapper.cpp
)

target_include_directories(pn532_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/rpi_tcptest
)
