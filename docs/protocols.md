<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk> -->

# Serial Modem Protocols Technical Specification & Reference Guide

This document provides a comprehensive technical breakdown of the **XMODEM**, **YMODEM**, and **ZMODEM** serial file transfer protocol specifications, packet structures, error detection mechanisms, control signals, and interaction sequence diagrams.

---

## 1. Protocol Comparison Summary

| Protocol Feature | XMODEM (Standard / 1K) | YMODEM | ZMODEM |
| :--- | :--- | :--- | :--- |
| **Primary Architecture** | Stop-and-wait block transfer | Stop-and-wait batch block transfer | Full-duplex streaming window |
| **Payload Block Sizes** | 128 bytes (`SOH`) / 1024 bytes (`STX`) | 128 bytes (`SOH`) / 1024 bytes (`STX`) | Variable subpackets (typically 1024 B) |
| **Error Checking** | 8-bit Checksum or 16-bit CRC CCITT | 16-bit CRC CCITT | 16-bit CRC CCITT or 32-bit CRC IEEE |
| **File Metadata** | None (Raw binary stream) | Block 0 (Filename, File Size in ASCII) | `ZFILE` frame (Filename, Size, Date, Mode) |
| **Batch Transfer** | Single file per session | Multi-file batch transfer | Multi-file batch transfer |
| **Crash Recovery** | None (Restarts from block 1) | None (Restarts from block 0) | Auto-Resume supported via `ZRPOS` offset |
| **Flow Control** | Inherent in stop-and-wait ACK | Inherent in stop-and-wait ACK | ZDLE byte escaping (`XON`/`XOFF` safe) |

---

## 2. XMODEM Protocol Technical Specification

### 2.1. Overview & Control Characters

XMODEM is a half-duplex, stop-and-wait protocol introduced by Ward Christensen in 1977. The receiver initiates transfer by sending `'C'` (for 16-bit CRC mode) or `NAK` (for 8-bit arithmetic checksum mode).

* `SOH` (`0x01`): Start of Header for 128-byte payload packets.
* `STX` (`0x02`): Start of Header for 1024-byte payload packets (XMODEM-1K).
* `EOT` (`0x04`): End of Transmission character.
* `ACK` (`0x06`): Positive Acknowledge signal sent by receiver.
* `NAK` (`0x15`): Negative Acknowledge signal requesting packet retransmission.
* `CAN` (`0x18`): Cancel signal (two consecutive `CAN` bytes abort transfer).
* `'C'` (`0x43`): Ascii character `'C'` requesting 16-bit CRC-CCITT mode.

### 2.2. Packet Frame Format

#### Standard 128-Byte / 1K Packet Frame Layout
```
+--------+----------+--------------+--------------------------+----------------+
| Header | Block #  | Inv Block #  | Data Payload             | Checksum / CRC |
| 1 Byte | 1 Byte   | 1 Byte       | 128 or 1024 Bytes        | 1 or 2 Bytes   |
+--------+----------+--------------+--------------------------+----------------+
| SOH/STX| (1..255) | ~(1..255)    | Raw Binary Data (Pad 1A) | CKSUM / CRC16  |
+--------+----------+--------------+--------------------------+----------------+
```

1. **Header Byte**: `SOH` (`0x01`) indicates 128-byte data block; `STX` (`0x02`) indicates 1024-byte data block.
2. **Block Number**: 8-bit unsigned integer starting at 1 and incrementing modulo 256 (`1..255, 0, 1...`).
3. **Inverted Block Number**: Bitwise inverse (`~Block #`) used for header integrity verification (`Block # + Inv Block # == 0xFF`).
4. **Data Payload**: 128 or 1024 bytes. Unfilled trailing bytes are padded with CP/M EOF marker `0x1A`.
5. **Checksum / CRC**:
   * Arithmetic Checksum: 1-byte sum of all data payload bytes modulo 256.
   * CRC-16 CCITT: 2-byte CRC (Big-Endian: High byte first, Low byte second) computed with polynomial `0x1021` and init `0x0000`.

### 2.3. Sequence & Timing Diagrams

#### XMODEM-CRC Single Block Transfer Sequence
```
  Host (Transmitter)                        Zephyr Device (Receiver)
       |                                                |
       | <------------------- 'C' (0x43) -------------- |  (1. Request CRC16)
       |                                                |
       | --- [SOH | 0x01 | 0xFE | Data128 | CRC16] ---> |  (2. Send Block 1)
       | <------------------- ACK (0x06) -------------- |  (3. ACK Block 1)
       |                                                |
       | --- [STX | 0x02 | 0xFD | Data1024 | CRC16] --> |  (4. Send Block 2 - 1K)
       | <------------------- ACK (0x06) -------------- |  (5. ACK Block 2)
       |                                                |
       | -------------------- EOT (0x04) -------------> |  (6. End of File)
       | <------------------- ACK (0x06) -------------- |  (7. Complete Session)
```

#### XMODEM Corrupted Packet & Retransmission
```
  Host (Transmitter)                        Zephyr Device (Receiver)
       |                                                |
       | --- [SOH | 0x01 | 0xFE | BadData | CRC16] ---> |  (Block 1 Corrupted)
       | <------------------- NAK (0x15) -------------- |  (CRC Mismatch -> NAK)
       |                                                |
       | --- [SOH | 0x01 | 0xFE | GoodData | CRC16] --> |  (Retransmit Block 1)
       | <------------------- ACK (0x06) -------------- |  (ACK Block 1)
```

---

## 3. YMODEM Protocol Technical Specification

### 3.1. Overview & Block 0 Header

YMODEM extends XMODEM-1K by providing batch file transfers, file metadata negotiation (filename, length), and multi-file sequencing.

Every YMODEM transfer begins with **Block 0** (Header Block), which contains the filename and file length formatted in ASCII decimal.

#### Block 0 Header Payload Layout (128 Bytes)
```
+------------------------+---+-----------------------+---+--------------------+
| Filename (ASCII String)|0x00| File Size in ASCII Dec|0x00| Padding (0x00)     |
+------------------------+---+-----------------------+---+--------------------+
| "firmware.bin"         |\0 | "1048576"             |\0 | Null Bytes (to 128)|
+------------------------+---+-----------------------+---+--------------------+
```

### 3.2. YMODEM EOT Handshake Rules

YMODEM requires a specific double-EOT handshake to end a file transfer:
1. When the transmitter finishes sending file blocks, it transmits `EOT`.
2. The receiver responds with `NAK` to acknowledge the first `EOT`.
3. The transmitter transmits a second `EOT`.
4. The receiver responds with `ACK` followed by `'C'` to request Block 0 of the next file in the batch.
5. If no more files remain, the transmitter sends an empty Block 0 (filename field starting with `0x00`).

### 3.3. YMODEM Batch Sequence Diagram

```
  Host (Transmitter)                        Zephyr Device (Receiver)
       |                                                |
       | <------------------- 'C' (0x43) -------------- |  (Request Header)
       | --- [SOH | 0x00 | 0xFF | Block0 Payload | CRC] |  (Send Block 0 Header)
       | <------------------- ACK (0x06) -------------- |  (ACK Block 0)
       | <------------------- 'C' (0x43) -------------- |  (Request File Data)
       |                                                |
       | --- [STX | 0x01 | 0xFE | Data1024 | CRC] ----> |  (Send Block 1 Data)
       | <------------------- ACK (0x06) -------------- |  (ACK Block 1)
       |                                                |
       | -------------------- EOT (0x04) -------------> |  (First EOT)
       | <------------------- NAK (0x15) -------------- |  (YMODEM NAK First EOT)
       | -------------------- EOT (0x04) -------------> |  (Second EOT)
       | <------------------- ACK (0x06) -------------- |  (ACK Second EOT)
       | <------------------- 'C' (0x43) -------------- |  (Request Next File)
       |                                                |
       | --- [SOH | 0x00 | 0xFF | Null 128B | CRC] ---> |  (Empty Block 0 = End)
       | <------------------- ACK (0x06) -------------- |  (Batch Completed)
```

---

## 4. ZMODEM Protocol Technical Specification

### 4.1. Overview & Framing Architecture

ZMODEM is a full-duplex streaming protocol designed by Chuck Forsberg in 1986. Unlike stop-and-wait protocols, ZMODEM streams data continuously without waiting for individual packet acknowledgments.

Key ZMODEM features:
* **HEX and Binary Frames**: Session control headers are encoded in ASCII HEX (`ZHEX`) or binary (`ZBIN`/`ZBIN32`).
* **ZDLE Escape Encoding**: Byte `0x18` (`ZDLE`) escapes control characters (`XON`, `XOFF`, `ZDLE`) to ensure transparent operation over 7-bit or software flow-controlled serial lines.
* **Auto-Resume (`ZRPOS`)**: If a transfer is interrupted, ZMODEM queries the receiver for the last valid byte offset and resumes streaming from that exact location.

### 4.2. ZMODEM Frame Types & Enders

#### Frame Types
* `ZRQINIT` (`0`): Request receive init.
* `ZRINIT` (`1`): Receiver capabilities and init flags.
* `ZFILE` (`4`): File header subpacket (filename, size, modification date).
* `ZRPOS` (`9`): Resume file offset position header.
* `ZDATA` (`10`): Data subpacket header.
* `ZEOF` (`11`): End of file notification header.
* `ZFIN` (`8`): Finish session header.

#### ZDLE Subpacket Enders
* `ZCRCE` (`'h'`): CRC follows; frame ends, header follows.
* `ZCRCG` (`'i'`): CRC follows; frame continues non-stop (streaming mode).
* `ZCRCQ` (`'j'`): CRC follows; receiver sends `ZACK` frame.
* `ZCRCW` (`'k'`): CRC follows; receiver sends `ZACK` frame, end of frame.

### 4.3. ZMODEM Transfer Sequence Diagram

```
  Host (Transmitter)                        Zephyr Device (Receiver)
       |                                                |
       | <------------------- * * ZDLE ZHEX ZRINIT ---- | (1. Receiver Init)
       | --- * * ZDLE ZBIN ZFILE [file info subpacket]-> | (2. File Header)
       | <------------------- * * ZDLE ZHEX ZRPOS 0 --- | (3. Resume Offset 0)
       |                                                |
       | --- * * ZDLE ZBIN ZDATA [Offset 0] -----------> | (4. Data Header)
       | --- [Data Subpacket 1] ZDLE ZCRCG [CRC] ------> | (5. Stream Data Chunk 1)
       | --- [Data Subpacket 2] ZDLE ZCRCG [CRC] ------> | (6. Stream Data Chunk 2)
       | --- [Data Subpacket 3] ZDLE ZCRCW [CRC] ------> | (7. Final Data Subpacket)
       | <------------------- * * ZDLE ZHEX ZACK ------ | (8. ACK Subpacket)
       |                                                |
       | --- * * ZDLE ZHEX ZEOF [Offset 3072] ---------> | (9. End of File)
       | <------------------- * * ZDLE ZHEX ZRINIT ---- | (10. ACK File Finish)
       |                                                |
       | --- * * ZDLE ZHEX ZFIN ----------------------> | (11. Finish Session)
       | <------------------- * * ZDLE ZHEX ZFIN ------ | (12. ACK Finish)
       | --- "OO" (Over and Out) ---------------------> | (13. Close Session)
```
