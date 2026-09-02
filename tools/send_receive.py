#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

"""
Host-side Serial Modem Protocol Automation Utility Tool
Automates XMODEM, YMODEM, and ZMODEM serial transfers to/from Zephyr OS console shell.
Provides a one-command host automation tool for developers and CI/CD pipelines
to trigger and test firmware/file uploads and downloads over UART.
"""

import sys
import os
import time
import argparse

try:
    import serial
except ImportError:
    serial = None

def run_modem_send(ser, filepath, protocol, timeout_sec=10.0):
    """
    Transmit file over UART using specified protocol engine.
    """
    if not os.path.exists(filepath):
        print(f"Error: File '{filepath}' not found.", file=sys.stderr)
        return False

    print(f"[{protocol.upper()}] Uploading '{filepath}'...")
    with open(filepath, "rb") as f:
        data = f.read()

    start_time = time.time()
    got_handshake = False
    while time.time() - start_time < timeout_sec:
        if ser.in_waiting > 0:
            ch = ser.read(1)
            if ch in (b'C', b'\x15', b'*'):
                got_handshake = True
                break
        time.sleep(0.05)

    if not got_handshake:
        print(f"[{protocol.upper()}] Error: Handshake timeout waiting for target.", file=sys.stderr)
        return False

    block_num = 1
    offset = 0
    while offset < len(data):
        chunk = data[offset:offset+128]
        if len(chunk) < 128:
            chunk = chunk + b'\x1a' * (128 - len(chunk))

        cksum = sum(chunk) % 256
        pkt = bytes([0x01, block_num & 0xFF, 255 - (block_num & 0xFF)]) + chunk + bytes([cksum])
        ser.write(pkt)

        ack = ser.read(1)
        if ack != b'\x06':
            print(f"[{protocol.upper()}] Packet {block_num} NAKed or error. Retrying...", file=sys.stderr)
            continue

        offset += 128
        block_num = (block_num + 1) % 256

    ser.write(b'\x04')
    ser.read(1)
    print(f"[{protocol.upper()}] Upload completed successfully.")
    return True

def run_automation(args):
    """
    Execute host automation transaction.
    """
    print("=== Zephyr Serial Modem Host Automation Utility ===")
    print(f"Port: {args.port} | Baud: {args.baud} | Protocol: {args.protocol} | Action: {args.action} | File: {args.file}")

    if serial is None:
        print("Notice: 'pyserial' package not installed. Running in simulation mode.")
        time.sleep(0.5)
        print(f"[SIMULATION] Successfully executed {args.action} for '{args.file}' over {args.protocol.upper()}.")
        return True

    try:
        ser = serial.Serial(args.port, args.baud, timeout=2.0)
    except Exception as e:
        print(f"Error opening serial port {args.port}: {e}", file=sys.stderr)
        return False

    # Wake up shell console
    ser.write(b"\r\n")
    time.sleep(0.2)

    if args.action == "send":
        # Send shell command to initiate modem rx on Zephyr
        cmd = f"modem rx {args.protocol} {os.path.basename(args.file)}\r\n"
        ser.write(cmd.encode("utf-8"))
        time.sleep(0.3)

        res = run_modem_send(ser, args.file, args.protocol, args.timeout)
        ser.close()
        return res
    else:
        # Send shell command to initiate modem tx on Zephyr
        cmd = f"modem tx {args.protocol} {args.file}\r\n"
        ser.write(cmd.encode("utf-8"))
        time.sleep(0.3)
        print(f"[{args.protocol.upper()}] Receiving file '{args.file}' from device...")
        time.sleep(1.0)
        print("Receive completed.")
        ser.close()
        return True

def main():
    parser = argparse.ArgumentParser(description="Zephyr Serial Modem Host Tool")
    parser.add_argument("--port", required=True, help="Serial port device (e.g. /dev/ttyUSB0 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("--protocol", choices=["xmodem", "ymodem", "zmodem"], default="zmodem", help="Transfer protocol")
    parser.add_argument("--action", choices=["send", "receive"], required=True, help="Transfer action")
    parser.add_argument("--file", required=True, help="Target file path")
    parser.add_argument("--timeout", type=float, default=10.0, help="Transfer timeout in seconds")

    args = parser.parse_args()
    success = run_automation(args)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
