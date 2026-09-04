/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Sample Application: ZMODEM Auto-Start Batch Upload to Device
 * Demonstrates background monitoring of incoming console stream for 'rz\r' sequence,
 * automatically switching to ZMODEM streaming receiver mode to upload multiple files.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>
#include <modem/zmodem.h>
#include <stdio.h>
#include <string.h>

static uint8_t autostart_buf[8] = {0};
static size_t autostart_idx = 0;

static bool check_zmodem_autostart(uint8_t byte)
{
    autostart_buf[autostart_idx % 8] = byte;
    autostart_idx++;
    if (autostart_idx >= 3) {
        size_t idx = (autostart_idx - 3) % 8;
        if (autostart_buf[idx] == 'r' &&
            autostart_buf[(idx + 1) % 8] == 'z' &&
            autostart_buf[(idx + 2) % 8] == '\r') {
            return true;
        }
    }
    return false;
}

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

static int zmodem_on_file_start(const zmodem_file_info_t *info, void *user_data)
{
    (void)user_data;
    printk("\n[ZMODEM] Receiving file: %s (%zu bytes)\n", info->filename, info->size);
    return 0;
}

static int zmodem_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
    (void)user_data;
    (void)buf;
    (void)offset;
    /* Process incoming file data stream chunk */
    return 0;
}

static void zmodem_on_file_end(const zmodem_file_info_t *info, zmodem_status_t status, void *user_data)
{
    (void)user_data;
    if (status == ZMODEM_OK) {
        printk("[ZMODEM] File '%s' transfer complete!\n", info->filename);
    } else {
        printk("[ZMODEM] File '%s' transfer error: %d\n", info->filename, (int)status);
    }
}

int main(void)
{
    printk("=== ZMODEM Auto-Start Batch Upload Sample ===\n");
    printk("Monitoring console input stream for ZMODEM 'rz\\r' trigger...\n");

    while (1) {
        int ch = console_getchar();
        if (ch >= 0) {
            uint8_t b = (uint8_t)ch;
            if (check_zmodem_autostart(b)) {
                printk("\n[ZMODEM] Auto-Start detected! Triggering streaming receiver...\n");

                zmodem_rx_callbacks_t cbs = {
                    .read_byte = app_read_byte,
                    .write_bytes = app_write_bytes,
                    .on_file_start = zmodem_on_file_start,
                    .on_data = zmodem_on_data,
                    .on_file_end = zmodem_on_file_end,
                    .user_data = NULL
                };

                zmodem_status_t status = zmodem_receive(&cbs);
                printk("[ZMODEM] Batch transfer ended with status: %d\n", (int)status);
                printk("Resuming console stream monitoring...\n");
            }
        }
        k_msleep(10);
    }

    return 0;
}
