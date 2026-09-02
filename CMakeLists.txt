# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
#
# CMakeLists.txt build file for Zephyr Console Modem Module.

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(src/crc.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_XMODEM src/xmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_YMODEM src/ymodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_ZMODEM src/zmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_CONSOLE src/zephyr_console_modem.c)
