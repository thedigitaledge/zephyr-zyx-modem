# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt build file for Zephyr ztest protocol unit tests.

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_zephyr_console_modem)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_sources(app PRIVATE
    src/main.c
    ../src/crc.c
    ../src/xmodem.c
    ../src/ymodem.c
    ../src/zmodem.c
    ../src/zephyr_console_modem.c
)
