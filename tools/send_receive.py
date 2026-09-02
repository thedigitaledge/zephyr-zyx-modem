#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

"""
Host-side Serial Modem Protocol Automation Utility Tool
Automates XMODEM, YMODEM, and ZMODEM serial transfers to/from Zephyr OS console shell.
"""

import sys
import argparse
import time

def main():
    parser = argparse.ArgumentParser(description="Zephyr Serial Modem Host Tool")
    parser.add_argument("--port", required=True, help="Serial port device (e.g. /dev/ttyUSB0 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("--protocol", choices=["xmodem", "ymodem", "zmodem"], default="zmodem", help="Transfer protocol")
    parser.add_argument("--action", choices=["send", "receive"], required=True, help="Transfer action")
    parser.add_argument("--file", required=True, help="Target file path")

    args = parser.parse_args()
    print(f"=== Zephyr Serial Modem Host Tool ===")
    print(f"Port: {args.port} | Baud: {args.baud} | Protocol: {args.protocol} | Action: {args.action} | File: {args.file}")
    print("Automation utility ready.")

if __name__ == "__main__":
    main()
