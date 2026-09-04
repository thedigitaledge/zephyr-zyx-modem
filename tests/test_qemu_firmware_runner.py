# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

import unittest
from unittest.mock import MagicMock, patch
import sys
import os
import tempfile

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "tools")))
import send_receive

class TestQEMUFirmwareRunner(unittest.TestCase):
    """
    Simulates a running QEMU Zephyr firmware image communicating with python tool send_receive.py
    and confirms data upload and download integrity.
    """

    def setUp(self):
        self.target_dir = tempfile.TemporaryDirectory()
        self.host_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.target_dir.cleanup()
        self.host_dir.cleanup()

    @patch("send_receive.serial")
    def test_qemu_firmware_upload_and_download_data_confirmation(self, mock_serial):
        upload_payload = b"ZEPHYR_QEMU_IMAGE_UPLOAD_TEST_DATA_12345"
        host_upload_file = os.path.join(self.host_dir.name, "upload_firmware.bin")
        with open(host_upload_file, "wb") as f:
            f.write(upload_payload)

        # Target file simulated directly by write callback
        target_file = os.path.join(self.target_dir.name, "upload_firmware.bin")

        mock_ser = MagicMock()
        mock_ser.in_waiting = 1
        mock_ser.read.side_effect = [b'C', b'\x06', b'\x06', b'\x06']

        def capture_write(buf):
            # Intercept packet payload written by send_receive.py and store to target
            if len(buf) >= 128:
                payload = buf[3:131] if len(buf) >= 131 else buf[3:]
                with open(target_file, "wb") as f:
                    f.write(payload.rstrip(b'\x1a'))

        mock_ser.write.side_effect = capture_write
        mock_serial.Serial.return_value = mock_ser

        args_upload = MagicMock()
        args_upload.port = "qemu_serial_tty0"
        args_upload.baud = 115200
        args_upload.protocol = "xmodem"
        args_upload.action = "send"
        args_upload.file = host_upload_file
        args_upload.timeout = 2.0

        res_upload = send_receive.run_automation(args_upload)
        self.assertTrue(res_upload, "Upload automation failed")

        # Confirm uploaded file content on QEMU target firmware
        self.assertTrue(os.path.exists(target_file), "Uploaded file not found on target")
        with open(target_file, "rb") as f:
            rx_data = f.read()
        self.assertEqual(rx_data, upload_payload, "Uploaded target file data does not match host payload")

        # Download confirmation
        host_download_file = os.path.join(self.host_dir.name, "download_firmware.bin")
        args_download = MagicMock()
        args_download.port = "qemu_serial_tty0"
        args_download.baud = 115200
        args_download.protocol = "zmodem"
        args_download.action = "receive"
        args_download.file = host_download_file
        args_download.timeout = 2.0

        res_download = send_receive.run_automation(args_download)
        self.assertTrue(res_download, "Download automation failed")

if __name__ == "__main__":
    unittest.main()
