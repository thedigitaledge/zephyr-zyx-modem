<!-- SPDX-License-Identifier: Apache-2.0 -->

# Console Shell Command Line Interface (CLI) Guide

This guide describes how to use the Zephyr Console Modem shell commands to receive and transmit files directly over the Zephyr console UART interface.

## Command Overview

The console modem module registers top-level `modem` shell commands and convenience short aliases:

| Long Command | Short Alias | Description | Syntax |
| :--- | :--- | :--- | :--- |
| `modem rx` | `mrx` | Receive file over serial console | `modem rx [x\|y\|z] [file]` |
| `modem tx` | `mtx` | Transmit file over serial console | `modem tx [x\|y\|z] <file>` |
| `modem config` | N/A | Inspect or update transfer parameters | `modem config [param val]` |

> **Note:** File transfer commands (`rx`, `tx`, `mrx`, `mtx`) require `CONFIG_FILE_SYSTEM=y` to be enabled in `prj.conf`.

---

## 1. Receiving Files (`modem rx` / `mrx`)

To upload a file from a host computer (PC) to the Zephyr target device:

1. On the Zephyr shell, start the receiver:
   ```bash
   mrx z /SD:/firmware.bin
   ```
   * First argument specifies protocol: `z` (ZMODEM, default), `y` (YMODEM), or `x` (XMODEM).
   * Second argument specifies target destination path on the Zephyr File System.

2. On the host computer, trigger the file transfer using your terminal emulator (`sz`, Minicom, Tera Term, ExtraPuTTY):
   ```bash
   sz firmware.bin > /dev/ttyUSB0 < /dev/ttyUSB0
   ```

---

## 2. Transmitting Files (`modem tx` / `mtx`)

To download a file from the Zephyr target device to a host computer:

1. On the host computer, prepare the receiver utility:
   ```bash
   rz > /dev/ttyUSB0 < /dev/ttyUSB0
   ```

2. On the Zephyr shell, start the transmission:
   ```bash
   mtx z /SD:/logs.txt
   ```
   * First argument specifies protocol: `z` (ZMODEM), `y` (YMODEM), or `x` (XMODEM).
   * Second argument specifies the source file path on the Zephyr File System.

---

## 3. Modem Configuration (`modem config`)

View or dynamically update modem transfer settings on the console:

### Inspect Current Settings
```bash
uart:~$ modem config
Modem Configuration:
  Packet Timeout:      3000 ms
  Byte Timeout:        1000 ms
  Max Retries:         10
  Inter-block Delay:   0 ms
  Handshake Delay:     1000 ms
  Overwrite Mode:      0
  Auto-Resume:         true
  Target Directory:    (root)
  Sync Interval:       10 blocks
```

### Update Transfer Parameters
```bash
# Update packet timeout to 5000 ms
uart:~$ modem config packet_timeout 5000

# Update max retries
uart:~$ modem config max_retries 15

# Set inter-block transmit delay to 10 ms
uart:~$ modem config inter_block_delay 10
```
