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
)

zephyr_library_sources_ifdef(CONFIG_MODEM_XMODEM src/xmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_YMODEM src/ymodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_ZMODEM src/zmodem.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_CONSOLE src/zephyr_console_modem.c)

zephyr_library_sources_ifdef(CONFIG_MODEM_STATS src/modem_stats.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_AUTO_START src/modem_autostart.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_USB_CDC_ACM src/modem_usb_cdc_acm.c)

zephyr_library_sources_ifdef(CONFIG_MODEM_BLE_NUS src/ble_nus_transport.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_NET_SOCKET src/net_socket_transport.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_DECOMPRESS src/stream_decompress.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_DELTA_UPDATE src/delta_update.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_SIGNATURE_VERIFY src/signature_verify.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_ENCRYPTED_STREAM src/encrypted_stream.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_SESSION_DISPATCHER src/session_dispatcher.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_LOG_ROTATION src/log_rotation.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_NFC src/nfc_transport.c)
