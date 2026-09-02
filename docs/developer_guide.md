<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Developer API Integration Guide

This guide describes how C developers can integrate the XMODEM, YMODEM, and ZMODEM protocol engines as a library into custom Zephyr OS applications, drivers, or secondary communication tasks.

For library deployment options (`west.yml`, out-of-tree CMake inclusion) and full Kconfig symbol references, see [docs/deployment.md](deployment.md).

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

---

## 4. Advanced Console Modem Runtime Control API

Header: `#include "zephyr_console_modem.h"`

Developers can query and modify console modem runtime configurations at runtime:

```c
#include "zephyr_console_modem.h"

void configure_modem_runtime(void)
{
    console_modem_settings_t cfg;
    console_modem_settings_get(&cfg);

    cfg.auto_start = true;
    cfg.progress_bar = true;
    cfg.async_storage = true;
    cfg.directory_transfers = true;
    cfg.ring_buffer = true;
    cfg.abort_key = true;
    cfg.abort_key_char = 27; /* ESC */
    cfg.flow_control = true;
    strncpy(cfg.flash_partition, "slot1", sizeof(cfg.flash_partition) - 1);

    console_modem_settings_set(&cfg);
}

void bind_secondary_uart(const struct device *uart_dev)
{
    console_modem_channel_t channel = {
        .uart_dev = uart_dev,
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .user_data = NULL
    };
    console_modem_bind_device(&channel);
}
```

---

## 5. Direct C Library Usage without Shell or File System Dependencies

This module can be compiled and used as a pure, lightweight C protocol library without enabling `CONFIG_SHELL` or `CONFIG_FILE_SYSTEM`.

### Application `prj.conf`:

```kconfig
CONFIG_CRC=y
CONFIG_MODEM_ZMODEM=y
# CONFIG_MODEM_CONSOLE is disabled by default when SHELL/FILE_SYSTEM are omitted
```

### Application `main.c`:

```c
#include <zephyr/kernel.h>
#include <modem/zmodem.h>

static int my_uart_read(uint8_t *b, uint32_t timeout_ms, void *user_data)
{
    /* Read byte directly from custom UART/SPI/Bluetooth driver */
    return 0;
}

static int my_uart_write(const uint8_t *buf, size_t len, void *user_data)
{
    /* Send buffer directly to custom UART/SPI/Bluetooth driver */
    return 0;
}

void main(void)
{
    zmodem_rx_callbacks_t cbs = {
        .read_byte = my_uart_read,
        .write_bytes = my_uart_write,
        .on_file_start = NULL,
        .on_data = NULL,
        .on_file_end = NULL,
        .user_data = NULL
    };

    /* Execute ZMODEM streaming receiver loop */
    zmodem_receive(&cbs);
}
```
