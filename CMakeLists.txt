# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt build file for Zephyr Console Modem Module.

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
    src/crc.c
    src/xmodem.c
    src/ymodem.c
    src/zmodem.c
    src/zephyr_console_modem.c
)
