<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Console Shell Command Line Interface (CLI) Guide

This guide provides a comprehensive user manual, Kconfig configuration reference, and parameter access guide for the Zephyr OS Console Modem protocols (XMODEM, YMODEM, ZMODEM).

For full technical specifications, packet formats, and sequence diagrams, refer to [docs/protocols.md](protocols.md).

---

## 1. Command Line Interface (CLI) Overview

The console modem module registers shell commands under the `modem` namespace, as well as short aliases for convenient terminal usage:

| Command | Short Alias | Description | Syntax | Required Kconfig |
| :--- | :--- | :--- | :--- | :--- |
| `modem rx` | `mrx` | Receive file over serial console | `modem rx [x\|y\|z] [file]` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_FILE_SYSTEM=y` |
| `modem tx` | `mtx` | Transmit file over serial console | `modem tx [x\|y\|z] <file>` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_FILE_SYSTEM=y` |
| `modem update` | N/A | Stream MCUBoot image update | `modem update [x\|y\|z] <file>` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_MODEM_MCUBOOT_UPDATE=y` |
| `modem config` | N/A | Inspect / update transfer settings | `modem config [param val]` | `CONFIG_MODEM_CONSOLE=y` |

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

---

## 3. Console Settings & Parameter Read/Write Permissions

Run `modem config` on the shell to view or modify runtime parameters:

```bash
uart:~$ modem config
Modem Configuration:
  Packet Timeout:      3000 ms
  Byte Timeout:        1000 ms
  Max Retries:         10
  Inter-block Delay:   0 ms
  Handshake Delay:     1000 ms
  Overwrite Mode:      0 (0=Overwrite, 1=Skip, 2=Abort)
  Auto-Resume:         true
  Target Directory:    (root)
  Sync Interval:       10 blocks
  Auto-Start:          true
  Async Storage:       true
  Progress Bar:        true
  Directory Transfers: true
  Ring Buffer Transport: true
  Abort Key Monitor:   true
```

### Parameter Permission Matrix

| Parameter Name | Read/Write Status | Description & Valid Values |
| :--- | :--- | :--- |
| `packet_timeout` | **Read/Write** | Packet response timeout in ms (e.g. `5000`) |
| `byte_timeout` | **Read/Write** | Inter-byte timeout in ms (e.g. `1000`) |
| `max_retries` | **Read/Write** | Maximum retry attempts (e.g. `10`) |
| `inter_block_delay` | **Read/Write** | Inter-block delay in ms (e.g. `10`) |
| `handshake_delay` | **Read/Write** | Handshake retry interval in ms (e.g. `1000`) |
| `overwrite_mode` | **Read/Write** | **Overwrite Policy**: `0` = Overwrite, `1` = Skip, `2` = Abort |
| `enable_resume` | **Read/Write** | Auto-resume toggle (`true`/`1` or `false`/`0`) |
| `target_dir` | **Read/Write** | Default target folder (e.g. `/SD:`) |
| `sync_interval` | **Read/Write** | Flash `fs_sync()` interval in blocks (0 = at file end) |
| `auto_start` | **Read/Write** | ZMODEM auto-start detection toggle (`true`/`1` or `false`/`0`) |
| `async_storage` | **Read/Write** | Async workqueue file write toggle (`true`/`1` or `false`/`0`) |
| `progress_bar` | **Read/Write** | Real-time shell progress bar toggle (`true`/`1` or `false`/`0`) |
| `directory_transfers` | **Read/Write** | Directory batch transfer toggle (`true`/`1` or `false`/`0`) |
| `ring_buffer` | **Read/Write** | Ring buffer transport adapter toggle (`true`/`1` or `false`/`0`) |
| `abort_key` | **Read/Write** | Terminal abort key monitor toggle (`true`/`1` or `false`/`0`) |
| `abort_char` | **Read/Write** | Terminal abort key ASCII byte value (e.g. `3` for Ctrl-C, `27` for ESC) |
| `flow_control` | **Read/Write** | Hardware RTS/CTS and software XON/XOFF flow control toggle (`true`/`false`) |
| `flash_partition` | **Read/Write** | Target raw flash area partition name (e.g. `slot1`) |
| `mcuboot_update` | **Read/Write** | MCUBoot firmware upgrade manager toggle (`true`/`false`) |
| `nvs_checkpoints` | **Read/Write** | NVS transfer auto-resume checkpoint toggle (`true`/`false`) |
| `crypto_stream` | **Read/Write** | In-flight payload streaming encryption toggle (`true`/`false`) |
| *`active_protocol`* | **Read-Only** | Active protocol selected during `rx`/`tx` command invocation |
| *`bytes_transferred`*| **Read-Only** | Counter tracking total bytes written/read during active session |
