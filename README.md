<!-- SPDX-License-Identifier: Apache-2.0 -->

# Zephyr Console Serial Modem Protocols (XMODEM / YMODEM / ZMODEM)

> **Developed using Google Jules**

This repository provides native Zephyr OS implementations of the standard serial file transfer protocols **XMODEM**, **YMODEM**, and **ZMODEM** designed to enable uploading and downloading files directly via the Zephyr OS console shell or embedded UART applications.

This project is licensed under the **Apache License Version 2.0** (`LICENSE`).

---

## Features

- **XMODEM Protocol**:
  - Receiver and Transmitter supporting standard 128-byte block mode with 8-bit checksum, XMODEM-CRC (128-byte block), and XMODEM-1K (1024-byte block).
  - Configurable default block size (`CONFIG_MODEM_XMODEM_BLOCK_SIZE_1024`) and default CRC negotiation (`CONFIG_MODEM_XMODEM_USE_CRC`).
  - Handles packet timeouts, retransmissions, sequence errors, and cancellation.

- **YMODEM Protocol**:
  - Batch file upload and download support.
  - Header block (Block 0) parsing for filename and file size in decimal ASCII.
  - Multi-file sequencing with double EOT handshaking and null block termination.

- **ZMODEM Protocol**:
  - High-performance streaming file uploads and downloads.
  - Support for HEX, BIN16, and ZDLE escape sequence processing (`CONFIG_MODEM_ZMODEM_ESCAPE_CTRL_CHARS`).
  - Support for 32-bit CRC (`CONFIG_MODEM_ZMODEM_USE_CRC32`).
  - Frame state machine handling `ZRQINIT`, `ZRINIT`, `ZFILE`, `ZDATA`, `ZEOF`, `ZFIN`, and `ZRPOS` auto-resume (`CONFIG_MODEM_ENABLE_RESUME`).

---

## Project Structure

```
├── include/
│   └── modem/
│       ├── xmodem.h               # XMODEM protocol API definition
│       ├── ymodem.h               # YMODEM protocol API definition
│       └── zmodem.h               # ZMODEM protocol API definition
├── src/
│   ├── crc.h                      # Zephyr sys/crc.h wrapper functions
│   ├── crc.c                      # Checksum implementation
│   ├── xmodem.c                   # XMODEM state machine and transfers
│   ├── ymodem.c                   # YMODEM state machine and transfers
│   ├── zmodem.c                   # ZMODEM state machine and transfers
│   ├── zephyr_console_modem.h     # Console modem internal definitions and configuration
│   └── zephyr_console_modem.c     # Zephyr console and shell binding
├── docs/                          # Detailed Guides
│   ├── cli.md                     # Shell Command Line Interface (CLI) Guide
│   └── developer_guide.md         # Developer C API Integration Guide
├── tests/                         # Zephyr ztest / Twister test suite
│   ├── src/
│   │   └── main.c                 # ztest protocol test cases
│   ├── CMakeLists.txt             # Zephyr test build configuration
│   ├── prj.conf                   # Test configuration (CONFIG_ZTEST, CONFIG_CRC)
│   └── testcase.yaml              # Twister test case specification
├── CMakeLists.txt                 # Zephyr module build file (zephyr_library)
├── Kconfig                        # Zephyr Kconfig options (selects CRC)
├── LICENSE                        # Apache License Version 2.0
├── zephyr/module.yml              # Zephyr OS module descriptor
└── README.md
```

---

## Zephyr OS Kconfig Options

Enable the module and protocol engines in `prj.conf`:

```kconfig
# Enable Zephyr Core Features
CONFIG_SHELL=y
CONFIG_CONSOLE=y
CONFIG_FILE_SYSTEM=y
CONFIG_CRC=y

# Enable Modem Module & Protocol Engines
CONFIG_MODEM_CONSOLE=y
CONFIG_MODEM_XMODEM=y
CONFIG_MODEM_YMODEM=y
CONFIG_MODEM_ZMODEM=y

# Modem Protocol Settings & Defaults
CONFIG_MODEM_XMODEM_BLOCK_SIZE_1024=y
CONFIG_MODEM_XMODEM_USE_CRC=y
CONFIG_MODEM_ZMODEM_USE_CRC32=y
CONFIG_MODEM_ZMODEM_ESCAPE_CTRL_CHARS=y

# Timeout, Retry, and Delay Defaults
CONFIG_MODEM_PACKET_TIMEOUT_MS=3000
CONFIG_MODEM_BYTE_TIMEOUT_MS=1000
CONFIG_MODEM_MAX_RETRIES=10
CONFIG_MODEM_INTER_BLOCK_DELAY_MS=0
CONFIG_MODEM_HANDSHAKE_DELAY_MS=1000

# Storage & File System Defaults
CONFIG_MODEM_FILE_OVERWRITE_MODE=0
CONFIG_MODEM_ENABLE_RESUME=y
CONFIG_MODEM_DEFAULT_TARGET_DIR=""
CONFIG_MODEM_SYNC_INTERVAL_BLOCKS=10
```

---

## Shell Integration Information

When `CONFIG_MODEM_CONSOLE=y` and `CONFIG_FILE_SYSTEM=y` are enabled, the following shell commands are available on the Zephyr console:

- **Receive File (Upload to Zephyr)**:
  `modem rx [x|y|z] [file]` or short alias `mrx [x|y|z] [file]`
- **Transmit File (Download from Zephyr)**:
  `modem tx [x|y|z] <file>` or short alias `mtx [x|y|z] <file>`
- **Modem Configuration**:
  `modem config [packet_timeout|byte_timeout|max_retries <val>]`

For full command line usage instructions, see [docs/cli.md](docs/cli.md).
For developer C API integration examples, see [docs/developer_guide.md](docs/developer_guide.md).

---

## Running Tests with Zephyr Twister

To execute the test suite using Zephyr's Twister test runner:

```bash
twister -T tests/ --integration
```
