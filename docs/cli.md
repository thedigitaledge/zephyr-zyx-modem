<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Console Shell Command Line Interface (CLI) Guide

This guide provides a user manual and parameter access reference for the Zephyr OS Console Modem shell interface (XMODEM, YMODEM, ZMODEM).

For full Kconfig configuration options and library deployment instructions, refer to [docs/developer_guide.md](developer_guide.md).
For technical specifications, packet formats, and sequence diagrams, refer to [docs/protocols.md](protocols.md).

---

## 1. Command Line Interface (CLI) Overview

The console modem module registers shell commands under the `modem` namespace, as well as short aliases for convenient terminal usage:

| Command | Short Alias | Description | Syntax | Required Kconfig |
| :--- | :--- | :--- | :--- | :--- |
| `modem rx` | `mrx` | Receive file over serial console or flash partition (`flash:<slot>`) | `modem rx [x\|y\|z] [file\|flash:<slot>]` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_FILE_SYSTEM=y` |
| `modem tx` | `mtx` | Transmit file or directory over serial console | `modem tx [x\|y\|z] <file\|dir>` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_FILE_SYSTEM=y` |
| `modem update` | N/A | Stream MCUBoot image update | `modem update [x\|y\|z] <file>` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_MODEM_MCUBOOT_UPDATE=y` |
| `modem stats` | N/A | View / reset transfer diagnostic counters | `modem stats [reset]` | `CONFIG_MODEM_CONSOLE=y` & `CONFIG_MODEM_STATS=y` |
| `modem config` | N/A | Inspect / update transfer settings | `modem config [param val]` | `CONFIG_MODEM_CONSOLE=y` |

---

## 2. Console Settings & Parameter Read/Write Permissions

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
