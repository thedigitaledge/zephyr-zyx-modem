# Zephyr Console Serial Modem Protocols (XMODEM / YMODEM / ZMODEM)

This repository provides full implementations of the standard serial file transfer protocols **XMODEM**, **YMODEM**, and **ZMODEM** designed to enable uploading and downloading data directly via the Zephyr OS console / shell or standalone embedded systems.

This module is designed strictly as a native Zephyr OS module and integrates directly with Zephyr's CRC service (`<zephyr/sys/crc.h>`), File System API (`<zephyr/fs/fs.h>`), Shell subsystem (`<zephyr/shell/shell.h>`), and `ztest` / Twister test framework.

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

- **Zephyr OS Console & Shell Integration**:
  - Zephyr shell commands:
    - Receive commands (requires `CONFIG_FILE_SYSTEM`): `modem rx [x|y|z] [file]` or short command `mrx [x|y|z] [file]`
    - Transmit commands (requires `CONFIG_FILE_SYSTEM`): `modem tx [x|y|z] <file>` or short command `mtx [x|y|z] <file>`
    - Configuration command: `modem config [param val]`
  - Configurable timeouts and pacing via Kconfig defaults (`CONFIG_MODEM_PACKET_TIMEOUT_MS`, `CONFIG_MODEM_BYTE_TIMEOUT_MS`, `CONFIG_MODEM_MAX_RETRIES`, `CONFIG_MODEM_INTER_BLOCK_DELAY_MS`, `CONFIG_MODEM_HANDSHAKE_DELAY_MS`).
  - Configurable file storage policy (`CONFIG_MODEM_FILE_OVERWRITE_MODE`, `CONFIG_MODEM_DEFAULT_TARGET_DIR`, `CONFIG_MODEM_SYNC_INTERVAL_BLOCKS`).
  - Full integration with Zephyr File System API (`<zephyr/fs/fs.h>`) and Zephyr CRC service (`<zephyr/sys/crc.h>`).

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

## Zephyr Integration

To use this module in a Zephyr application:

1. Add this repository to your Zephyr project modules or workspace.
2. Enable the module in `prj.conf`:
   ```kconfig
   CONFIG_SHELL=y
   CONFIG_CONSOLE=y
   CONFIG_FILE_SYSTEM=y
   CONFIG_CRC=y
   CONFIG_MODEM_CONSOLE=y
   CONFIG_MODEM_XMODEM=y
   CONFIG_MODEM_YMODEM=y
   CONFIG_MODEM_ZMODEM=y
   CONFIG_MODEM_XMODEM_BLOCK_SIZE_1024=y
   CONFIG_MODEM_XMODEM_USE_CRC=y
   CONFIG_MODEM_ZMODEM_USE_CRC32=y
   CONFIG_MODEM_ZMODEM_ESCAPE_CTRL_CHARS=y
   CONFIG_MODEM_PACKET_TIMEOUT_MS=3000
   CONFIG_MODEM_BYTE_TIMEOUT_MS=1000
   CONFIG_MODEM_MAX_RETRIES=10
   CONFIG_MODEM_INTER_BLOCK_DELAY_MS=0
   CONFIG_MODEM_HANDSHAKE_DELAY_MS=1000
   CONFIG_MODEM_FILE_OVERWRITE_MODE=0
   CONFIG_MODEM_ENABLE_RESUME=y
   CONFIG_MODEM_DEFAULT_TARGET_DIR=""
   CONFIG_MODEM_SYNC_INTERVAL_BLOCKS=10
   ```
3. Use terminal tools like `sx` / `sb` / `sz` or `rx` / `rb` / `rz` (from `lrzsz` or Minicom) to upload and download files directly over the Zephyr shell console.

## Running Tests with Zephyr Twister

To execute the test suite using Zephyr's Twister test runner:

```bash
twister -T tests/ --integration
```
