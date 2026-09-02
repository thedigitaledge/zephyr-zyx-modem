# Zephyr Console Serial Modem Protocols (XMODEM / YMODEM / ZMODEM)

This repository provides full implementations of the standard serial file transfer protocols **XMODEM**, **YMODEM**, and **ZMODEM** designed to enable uploading and downloading data directly via the Zephyr OS console / shell or standalone embedded systems.

## Features

- **XMODEM Protocol**:
  - Support for Standard 128-byte block mode with 8-bit arithmetic checksum.
  - Support for XMODEM-CRC (128-byte block with 16-bit CRC CCITT).
  - Support for XMODEM-1K (1024-byte block with 16-bit CRC CCITT).
  - Handles packet timeouts, retransmissions, sequence errors, and cancellation.

- **YMODEM Protocol**:
  - Batch file transfer support.
  - Header block (Block 0) parsing for filename and file size in decimal ASCII.
  - Multi-file sequencing with double EOT handshaking and null block termination.

- **ZMODEM Protocol**:
  - High-performance streaming file transfers.
  - Support for HEX, BIN16, and ZDLE escape sequence processing.
  - Frame state machine handling `ZRQINIT`, `ZRINIT`, `ZFILE`, `ZDATA`, `ZEOF`, and `ZFIN`.

- **Zephyr OS Console & Shell Integration**:
  - Zephyr shell commands registered under `modem`:
    - `modem rx [file]` / `rx [file]`
    - `modem ry [file]` / `ry [file]`
    - `modem rz [file]` / `rz [file]`
  - Standardized transport and storage callbacks for seamless integration with Zephyr console UART driver and file systems.

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
   CONFIG_MODEM_CONSOLE=y
   CONFIG_MODEM_XMODEM=y
   CONFIG_MODEM_YMODEM=y
   CONFIG_MODEM_ZMODEM=y
   ```
3. Use terminal tools like `sx` / `sb` / `sz`, `extraputty`, `minicom`, or `Tera Term` to transfer files directly via the Zephyr shell console.

## Building and Running Unit Tests

To build and execute unit tests locally:

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

All unit tests cover CRC calculation, XMODEM single/multi-block receive, YMODEM file header parsing, and ZMODEM frame handshake sequences.
