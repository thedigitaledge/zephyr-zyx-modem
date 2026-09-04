# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

import unittest
from unittest.mock import MagicMock, patch, mock_open
import sys
import os
import io

# Ensure tools/ is in python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "tools")))
import send_receive

class TestSendReceive(unittest.TestCase):

    def test_run_modem_send_file_not_found(self):
        ser = MagicMock()
        res = send_receive.run_modem_send(ser, "non_existent_file.txt", "zmodem")
        self.assertFalse(res)

    @patch("builtins.open", new_callable=mock_open, read_data=b"hello world payload")
    @patch("os.path.exists", return_value=True)
    def test_run_modem_send_handshake_timeout(self, mock_exists, mock_file):
        ser = MagicMock()
        ser.in_waiting = 0
        res = send_receive.run_modem_send(ser, "test.bin", "xmodem", timeout_sec=0.1)
        self.assertFalse(res)

    @patch("builtins.open", new_callable=mock_open, read_data=b"test data 12345")
    @patch("os.path.exists", return_value=True)
    def test_run_modem_send_success(self, mock_exists, mock_file):
        ser = MagicMock()
        ser.in_waiting = 1
        ser.read.side_effect = [b'C', b'\x06', b'\x06']
        res = send_receive.run_modem_send(ser, "test.bin", "xmodem", timeout_sec=1.0)
        self.assertTrue(res)

    @patch("send_receive.serial", None)
    def test_run_automation_simulation_mode(self):
        args = MagicMock()
        args.port = "/dev/ttyUSB0"
        args.baud = 115200
        args.protocol = "zmodem"
        args.action = "send"
        args.file = "test.txt"

        res = send_receive.run_automation(args)
        self.assertTrue(res)

    @patch("send_receive.serial")
    @patch("send_receive.run_modem_send", return_value=True)
    def test_run_automation_send(self, mock_send, mock_serial):
        ser_instance = MagicMock()
        mock_serial.Serial.return_value = ser_instance

        args = MagicMock()
        args.port = "/dev/ttyUSB0"
        args.baud = 115200
        args.protocol = "xmodem"
        args.action = "send"
        args.file = "test.bin"
        args.timeout = 5.0

        res = send_receive.run_automation(args)
        self.assertTrue(res)
        ser_instance.write.assert_called()

    @patch("send_receive.serial")
    def test_run_automation_receive(self, mock_serial):
        ser_instance = MagicMock()
        mock_serial.Serial.return_value = ser_instance

        args = MagicMock()
        args.port = "/dev/ttyUSB0"
        args.baud = 115200
        args.protocol = "ymodem"
        args.action = "receive"
        args.file = "dl.bin"

        res = send_receive.run_automation(args)
        self.assertTrue(res)

    def test_main_cli(self):
        test_args = ["send_receive.py", "--port", "COM3", "--action", "send", "--file", "fw.bin"]
        with patch.object(sys, "argv", test_args):
            with patch("send_receive.run_automation", return_value=True) as mock_auto:
                with self.assertRaises(SystemExit) as cm:
                    send_receive.main()
                self.assertEqual(cm.exception.code, 0)
                mock_auto.assert_called_once()

if __name__ == "__main__":
    unittest.main()
