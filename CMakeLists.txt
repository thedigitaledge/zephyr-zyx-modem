# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
#
# CMakeLists.txt build file for Zephyr Console Modem Module.

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
    src/crc.c
    src/modem_flash_helper.c
    src/mcuboot_validate.c
    src/ble_nus_transport.c
    src/net_socket_transport.c
    src/stream_decompress.c
    src/delta_update.c
    src/signature_verify.c
    src/encrypted_stream.c
    src/session_dispatcher.c
    src/log_rotation.c
)
zephyr_library_sources_ifdef(CONFIG_MODEM_XMODEM src/xmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_YMODEM src/ymodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_ZMODEM src/zmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_CONSOLE src/zephyr_console_modem.c)
