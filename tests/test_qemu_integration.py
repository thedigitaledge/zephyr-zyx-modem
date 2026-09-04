# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

import unittest
from unittest.mock import MagicMock, patch
import sys
import os
import tempfile

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "tools")))
import send_receive

class TestQEMUSerialIntegration(unittest.TestCase):
    """
    Simulates QEMU Zephyr serial console upload and download automation with tools/send_receive.py.
    """

    def setUp(self):
        self.test_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.test_dir.cleanup()

    @patch("send_receive.serial")
    def test_qemu_upload_integration(self, mock_serial):
        test_file = os.path.join(self.test_dir.name, "upload_test.bin")
        with open(test_file, "wb") as f:
            f.write(b"QEMU Upload Test Payload Data" * 5)

        # Mock serial object representing QEMU UART console connection
        mock_ser = MagicMock()
        mock_ser.in_waiting = 1
        # QEMU console responds with 'C' (handshake) then ACK (\x06)
        mock_ser.read.side_effect = [b'C', b'\x06', b'\x06', b'\x06']
        mock_serial.Serial.return_value = mock_ser

        args = MagicMock()
        args.port = "qemu_pty_serial0"
        args.baud = 115200
        args.protocol = "xmodem"
        args.action = "send"
        args.file = test_file
        args.timeout = 2.0

        res = send_receive.run_automation(args)
        self.assertTrue(res)

        # Verify QEMU shell modem rx trigger command was issued
        written_bytes = b"".join(call[0][0] for call in mock_ser.write.call_args_list if call[0])
        self.assertIn(b"modem rx xmodem upload_test.bin", written_bytes)

    @patch("send_receive.serial")
    def test_qemu_download_integration(self, mock_serial):
        test_file = os.path.join(self.test_dir.name, "download_test.bin")

        mock_ser = MagicMock()
        mock_ser.in_waiting = 1
        mock_serial.Serial.return_value = mock_ser

        args = MagicMock()
        args.port = "qemu_pty_serial0"
        args.baud = 115200
        args.protocol = "zmodem"
        args.action = "receive"
        args.file = test_file
        args.timeout = 2.0

        res = send_receive.run_automation(args)
        self.assertTrue(res)

        # Verify QEMU shell modem tx trigger command was issued
        written_bytes = b"".join(call[0][0] for call in mock_ser.write.call_args_list if call[0])
        self.assertIn(b"modem tx zmodem", written_bytes)
        self.assertIn(b"download_test.bin", written_bytes)

if __name__ == "__main__":
    unittest.main()
