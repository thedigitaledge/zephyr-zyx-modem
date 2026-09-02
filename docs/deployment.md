<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Deployment and Library Integration Guide

This guide covers deployment options, Zephyr library integration patterns, and the complete Kconfig configuration reference for the Zephyr OS Serial Modem Protocols module (XMODEM, YMODEM, ZMODEM).

---

## 1. Library Integration Patterns

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

### Option C: Pure Protocol Library Usage (No Shell / VFS Dependency)

To compile and use the protocol state machines (`xmodem.c`, `ymodem.c`, `zmodem.c`) as standalone C libraries without pulling in Zephyr shell commands or file system overhead:

```kconfig
# prj.conf
CONFIG_CRC=y
CONFIG_MODEM_ZMODEM=y
# CONFIG_MODEM_CONSOLE=n # Disable shell integration to save flash
# CONFIG_MODEM_XMODEM=n
# CONFIG_MODEM_YMODEM=n
```

---

## 2. Kconfig Configuration Reference

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
