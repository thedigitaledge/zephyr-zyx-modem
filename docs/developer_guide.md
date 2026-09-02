<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Developer API, Deployment, and Kconfig Integration Guide

This guide describes how developers can integrate the XMODEM, YMODEM, and ZMODEM protocol engines into custom Zephyr OS applications, drivers, or secondary communication tasks, deploy the module as a library, and configure Kconfig build options.

---

## 1. Library Deployment and Integration Patterns

This project can be integrated into custom Zephyr applications as an out-of-tree module or static library.

### Option A: Zephyr Module (`west.yml`)

Add this repository to your West manifest file (`west.yml`):

```yaml
manifest:
  projects:
    - name: zephyr-modem-protocols
      url: https://github.com/cwest/zephyr-modem-protocols
      revision: main
      path: modules/lib/zephyr-modem-protocols
```

### Option B: CMake Out-of-Tree Inclusion (`CMakeLists.txt`)

In your application's root `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_embedded_app)

# Include the modem protocols library
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/zephyr-modem-protocols zephyr_modem)

target_sources(app PRIVATE src/main.c)
```

### Option C: Standalone C Library Usage (No Shell / VFS Dependency)

To compile and use the protocol state machines (`xmodem.c`, `ymodem.c`, `zmodem.c`) as pure standalone C libraries without pulling in Zephyr shell commands or file system overhead:

```kconfig
# prj.conf
CONFIG_CRC=y
CONFIG_MODEM_ZMODEM=y
# CONFIG_MODEM_CONSOLE is disabled by default when SHELL/FILE_SYSTEM are omitted
```

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

---

## 2. Architecture & Callbacks Design

All protocol state machines (`xmodem.c`, `ymodem.c`, `zmodem.c`) are abstract and transport-agnostic. They communicate with physical channels (UART, SPI, USB, pipes) and storage mechanisms (VFS, Flash, RAM) via explicit function pointer callback structures.

---

## 3. C API Protocol Engines

### XMODEM Integration (`#include <modem/xmodem.h>`)

```c
#include <modem/xmodem.h>

static int app_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    return uart_poll_read_byte(byte) ? 0 : -1;
}

static int app_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(buf[i]);
    }
    return 0;
}

static int app_store_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
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
}
```

### YMODEM Integration (`#include <modem/ymodem.h>`)

```c
#include <modem/ymodem.h>

static int ymodem_on_file_start(const ymodem_file_info_t *info, void *user_data)
{
    printf("Incoming YMODEM File: %s (%zu bytes)\n", info->filename, info->size);
    return 0;
}

static int ymodem_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
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

### ZMODEM Integration (`#include <modem/zmodem.h>`)

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

Developers can query and modify console modem runtime configurations and bind secondary UART devices:

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

## 5. Kconfig Configuration Reference

Below are the Kconfig configuration symbols defined in `Kconfig`, including their default values and detailed explanations:

### Protocol Engine Selection
* **`CONFIG_MODEM_ZMODEM`** (bool, default `y`): Enables the primary ZMODEM protocol engine (streaming binary/hex frames, ZDLE escape).
* **`CONFIG_MODEM_XMODEM`** (bool, default `n`): Enables the legacy XMODEM protocol engine (128-byte and 1K blocks, CRC16 and Checksum). Set to `n` by default to reduce compiled binary size.
* **`CONFIG_MODEM_YMODEM`** (bool, default `n`): Enables the legacy YMODEM protocol engine (Block 0 file metadata, batch transfer). Set to `n` by default to reduce compiled binary size.

### Protocol Engine Defaults
* **`CONFIG_MODEM_XMODEM_BLOCK_SIZE_1024`** (bool, default `y`): Use 1024-byte payload blocks by default for XMODEM (XMODEM-1K).
* **`CONFIG_MODEM_XMODEM_USE_CRC`** (bool, default `y`): Use 16-bit CCITT CRC by default for XMODEM negotiation (`'C'`).
* **`CONFIG_MODEM_ZMODEM_USE_CRC32`** (bool, default `y`): Enable 32-bit CRC (`ZBIN32`) for ZMODEM streaming.
* **`CONFIG_MODEM_ZMODEM_ESCAPE_CTRL_CHARS`** (bool, default `y`): Escape `XON`/`XOFF` control characters via `ZDLE` to support software flow control.

### Timeout, Pacing, and Retry Defaults
* **`CONFIG_MODEM_PACKET_TIMEOUT_MS`** (int, default `3000`): Default packet response timeout in milliseconds.
* **`CONFIG_MODEM_BYTE_TIMEOUT_MS`** (int, default `1000`): Default inter-byte reception timeout in milliseconds.
* **`CONFIG_MODEM_MAX_RETRIES`** (int, default `10`): Maximum retry attempts for corrupted or lost packets.
* **`CONFIG_MODEM_INTER_BLOCK_DELAY_MS`** (int, default `0`): Configurable delay inserted between transmitted blocks in milliseconds.
* **`CONFIG_MODEM_HANDSHAKE_DELAY_MS`** (int, default `1000`): Delay between initial handshake retry attempts in milliseconds.

### File Storage Policies
* **`CONFIG_MODEM_FILE_OVERWRITE_MODE`** (int, default `0`): Policy when destination file exists on target file system:
  * `0` = **Always Overwrite** existing file.
  * `1` = **Skip** file transfer if file exists.
  * `2` = **Abort** batch transfer if file exists.
* **`CONFIG_MODEM_ENABLE_RESUME`** (bool, default `y`, depends on `MODEM_ZMODEM`): Enable ZMODEM `ZRPOS` auto-resume for interrupted transfers.
* **`CONFIG_MODEM_DEFAULT_TARGET_DIR`** (string, default `""`): Default storage mount point directory path.
* **`CONFIG_MODEM_SYNC_INTERVAL_BLOCKS`** (int, default `10`): Interval in received payload blocks between calling storage `fs_sync()`.

### Advanced Feature Options
* **`CONFIG_MODEM_AUTO_START`** (bool, default `y`): Background monitoring of console stream for ZMODEM initiation sequence (`rz\r`).
* **`CONFIG_MODEM_ASYNC_STORAGE`** (bool, default `y`): Offload file write operations to background Zephyr workqueues (`k_work`).
* **`CONFIG_MODEM_PROGRESS_BAR`** (bool, default `y`): Real-time shell console progress bar and throughput (KB/s) indicator.
* **`CONFIG_MODEM_DIRECTORY_TRANSFERS`** (bool, default `y`, depends on `MODEM_YMODEM || MODEM_ZMODEM`): Allow transmitting all files in a directory via batch transfers.
* **`CONFIG_MODEM_RING_BUFFER`** (bool, default `y`): Non-blocking interrupt-driven ring buffer UART adapter.
* **`CONFIG_MODEM_ABORT_KEY`** (bool, default `y`): Monitor console stream for Ctrl-C (`0x03`) or double CAN (`0x18 0x18`) user cancellation sequences.
* **`CONFIG_MODEM_ABORT_KEY_CHAR`** (int, default `3`): ASCII byte value used as terminal abort key (e.g. `3` for Ctrl-C / `0x03`, `27` for ESC / `0x1B`, `24` for CAN / `0x18`).
* **`CONFIG_MODEM_ZMODEM_RLE`** (bool, default `y`): Enable ZMODEM Run-Length Encoding (RLE) byte stream compression.
* **`CONFIG_MODEM_FLOW_CONTROL`** (bool, default `y`): RTS/CTS hardware and XON/XOFF software flow control support.
* **`CONFIG_MODEM_FLASH_PARTITION`** (bool, default `y`): Direct streaming uploads to raw flash area partitions (MCUBoot) without VFS.
* **`CONFIG_MODEM_HW_CRC`** (bool, default `n`): Hardware CRC driver accelerator offloading.
* **`CONFIG_MODEM_MCUBOOT_UPDATE`** (bool, default `y`): MCUBoot dual-bank image upgrade manager (`modem update` command).
* **`CONFIG_MODEM_NVS_CHECKPOINTS`** (bool, default `y`): Persist transfer progress checkpoints to NVS / Settings subsystem.
* **`CONFIG_MODEM_CRYPTO_STREAM`** (bool, default `n`): In-flight payload streaming encryption (PSA Crypto / MbedTLS).
* **`CONFIG_MODEM_STRESS_TEST`** (bool, default `y`): Fault injection test harness helpers for simulating noisy serial lines.
