# Zephyr Console Serial Modem Protocols (XMODEM / YMODEM / ZMODEM)

This repository provides full implementations of the standard serial file transfer protocols **XMODEM**, **YMODEM**, and **ZMODEM** designed to enable uploading and downloading data directly via the Zephyr OS console / shell or standalone embedded systems.

## Features

- **XMODEM Protocol**:
  - Receiver and Transmitter supporting standard 128-byte block mode with 8-bit checksum, XMODEM-CRC (128-byte block), and XMODEM-1K (1024-byte block).
  - Handles packet timeouts, retransmissions, sequence errors, and cancellation.

- **YMODEM Protocol**:
  - Batch file upload and download support.
  - Header block (Block 0) parsing for filename and file size in decimal ASCII.
  - Multi-file sequencing with double EOT handshaking and null block termination.

- **ZMODEM Protocol**:
  - High-performance streaming file uploads and downloads.
  - Support for HEX, BIN16, and ZDLE escape sequence processing.
  - Frame state machine handling `ZRQINIT`, `ZRINIT`, `ZFILE`, `ZDATA`, `ZEOF`, `ZFIN`, and `ZRPOS` position offsets.

- **Zephyr OS Console & Shell Integration**:
  - Zephyr shell commands registered under `modem`:
    - Receive commands: `modem rx [file]`, `modem ry [file]`, `modem rz [file]` (aliases: `rx`, `ry`, `rz`)
    - Transmit commands: `modem sx <file>`, `modem sy <file>`, `modem sz <file>` (aliases: `sx`, `sy`, `sz`)
  - Integration with Zephyr File System API (`<zephyr/fs/fs.h>`) and Zephyr CRC service (`<zephyr/sys/crc.h>`).

## Project Structure

```
├── include/
│   └── modem/
│       ├── crc.h                  # CRC-16 and CRC-32 math functions
│       ├── xmodem.h               # XMODEM protocol API definition
│       ├── ymodem.h               # YMODEM protocol API definition
│       ├── zmodem.h               # ZMODEM protocol API definition
│       └── zephyr_console_modem.h # Zephyr console & shell integration
├── src/
│   ├── crc.c                      # CRC computation implementations
│   ├── xmodem.c                   # XMODEM state machine and transfers
│   ├── ymodem.c                   # YMODEM state machine and transfers
│   ├── zmodem.c                   # ZMODEM state machine and transfers
│   └── zephyr_console_modem.c     # Zephyr console and shell binding
├── tests/                         # Comprehensive C unit tests
│   ├── test_crc.c
│   ├── test_xmodem.c
│   ├── test_ymodem.c
│   └── test_zmodem.c
├── CMakeLists.txt                 # CMake build configuration
├── Kconfig                        # Zephyr Kconfig options
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
   ```
3. Use terminal tools like `sx` / `sb` / `sz` or `rx` / `rb` / `rz` (from `lrzsz` or Minicom) to upload and download files directly over the Zephyr shell console.

## Building and Running Unit Tests

To build and execute unit tests locally:

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
