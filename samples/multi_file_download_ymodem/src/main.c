/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Sample Application: YMODEM Multi-File Download from Device
 * Demonstrates transmitting multiple data log files from target storage
 * using YMODEM batch transfer protocol.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>
#include <modem/ymodem.h>
#include <stdio.h>
#include <string.h>

static const char *demo_files[] = {
    "sensor_log_01.csv",
    "system_event.log"
};

static const char *demo_data[] = {
    "timestamp,temp,humidity\n1700000000,23.5,45.2\n1700000010,23.6,45.1\n",
    "[INFO] System initialized\n[INFO] Sensor calibration OK\n"
};

static int app_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
    int ch = console_getchar();
    if (ch < 0) return -1;
    *byte = (uint8_t)ch;
    return 0;
}

static int app_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    for (size_t i = 0; i < len; i++) {
        console_putchar(buf[i]);
    }
    return 0;
}

static int ymodem_tx_get_file_info(size_t file_index, ymodem_file_info_t *info, void *user_data)
{
    (void)user_data;
    if (file_index >= 2) return -1; /* End of batch */

    strncpy(info->filename, demo_files[file_index], sizeof(info->filename) - 1);
    info->filename[sizeof(info->filename) - 1] = '\0';
    info->size = strlen(demo_data[file_index]);
    return 0;
}

static int ymodem_tx_read_data(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    if (file_index >= 2) return -1;
    size_t file_len = strlen(demo_data[file_index]);
    if (offset >= file_len) return 0;

    size_t read_bytes = file_len - offset;
    if (read_bytes > len) read_bytes = len;
    memcpy(buf, demo_data[file_index] + offset, read_bytes);
    return (int)read_bytes;
}

int main(void)
{
    printk("=== YMODEM Multi-File Download Sample ===\n");
    printk("Initiating YMODEM batch transmit for 2 log files...\n");

    ymodem_tx_callbacks_t cbs = {
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .get_file_info = ymodem_tx_get_file_info,
        .read_data = ymodem_tx_read_data,
        .user_data = NULL
    };

    ymodem_status_t status = ymodem_transmit(&cbs);

    if (status == YMODEM_OK) {
        printk("\nYMODEM batch transmit completed successfully!\n");
    } else {
        printk("\nYMODEM batch transmit failed! (Error code: %d)\n", (int)status);
    }

    return 0;
}
