cmake_minimum_required(VERSION 3.10)
project(zephyr_console_modem C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(MODEM_SOURCES
    src/crc.c
    src/xmodem.c
    src/ymodem.c
    src/zmodem.c
    src/zephyr_console_modem.c
)

add_library(modem STATIC ${MODEM_SOURCES})

enable_testing()

add_executable(test_crc tests/test_crc.c)
target_link_libraries(test_crc modem)
add_test(NAME test_crc COMMAND test_crc)

add_executable(test_xmodem tests/test_xmodem.c)
target_link_libraries(test_xmodem modem)
add_test(NAME test_xmodem COMMAND test_xmodem)

add_executable(test_ymodem tests/test_ymodem.c)
target_link_libraries(test_ymodem modem)
add_test(NAME test_ymodem COMMAND test_ymodem)

add_executable(test_zmodem tests/test_zmodem.c)
target_link_libraries(test_zmodem modem)
add_test(NAME test_zmodem COMMAND test_zmodem)
