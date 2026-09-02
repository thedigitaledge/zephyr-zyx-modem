<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Developer API Integration Guide

This guide describes how C developers can integrate the XMODEM, YMODEM, and ZMODEM protocol engines into custom Zephyr OS applications and drivers.

## Architecture & Callbacks Design

All protocol state machines (`xmodem.c`, `ymodem.c`, `zmodem.c`) are abstract and transport-agnostic. They communicate with physical channels (UART, SPI, USB, pipes) and storage mechanisms (VFS, Flash, RAM) via explicit function pointer callback structures.

---

## 1. XMODEM Integration

Header: `#include <modem/xmodem.h>`

### Receiver Example

```c
#include <modem/xmodem.h>

static int app_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    /* Read 1 byte from UART or channel within timeout_ms */
    return uart_poll_read_byte(byte) ? 0 : -1;
}

static int app_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    /* Write buffer to UART or channel */
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(buf[i]);
    }
    return 0;
}

static int app_store_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    /* Save payload block to flash / storage */
    return flash_write_block(block_num, buf, len);
}

void receive_file_xmodem(void)
{
    xmodem_callbacks_t cbs = {
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .data_cb = app_store_data_cb,
        .user_data = NULL
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.mode = XMODEM_MODE_1K; /* XMODEM-1K CRC */

    size_t total_rx = 0;
    xmodem_status_t res = xmodem_receive(&cbs, &cfg, &total_rx);
    if (res == XMODEM_OK) {
        /* Success */
    }
}
```

---

## 2. YMODEM Integration

Header: `#include <modem/ymodem.h>`

YMODEM handles multi-file batch transfers and negotiates filename and file size in Block 0:

```c
#include <modem/ymodem.h>

static int ymodem_on_file_start(const ymodem_file_info_t *info, void *user_data)
{
    printf("Incoming YMODEM File: %s (%zu bytes)\n", info->filename, info->size);
    return 0;
}

static int ymodem_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
    /* Process file data chunk at offset */
    return 0;
}

void receive_ymodem_batch(void)
{
    ymodem_rx_callbacks_t cbs = {
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .on_file_start = ymodem_on_file_start,
        .on_data = ymodem_on_data,
        .on_file_end = NULL,
        .user_data = NULL
    };

    ymodem_status_t res = ymodem_receive(&cbs);
}
```

---

## 3. ZMODEM Integration

Header: `#include <modem/zmodem.h>`

ZMODEM provides high-speed streaming transfers with auto-resume support:

```c
#include <modem/zmodem.h>

void receive_zmodem_stream(void)
{
    zmodem_rx_callbacks_t cbs = {
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .on_file_start = zmodem_on_file_start,
        .on_data = zmodem_on_data,
        .on_file_end = NULL,
        .user_data = NULL
    };

    zmodem_status_t res = zmodem_receive(&cbs);
}
```
