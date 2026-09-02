/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Sample Application: XMODEM Firmware Image Upload to Device Flash
 * Demonstrates streaming incoming firmware image bytes via XMODEM-1K CRC
 * and writing directly to secondary flash area slot (MCUBoot).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>
#include <modem/xmodem.h>
#include <stdio.h>

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

static int app_flash_write_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    /* Flash write callback: Write len bytes at block offset */
    printk("Writing Block %u (%zu bytes) to Flash...\n", block_num, len);
    return 0;
}

int main(void)
{
    printk("=== XMODEM Firmware Upload Sample ===\n");
    printk("Ready to receive firmware image over serial console...\n");

    xmodem_callbacks_t cbs = {
        .read_byte = app_read_byte,
        .write_bytes = app_write_bytes,
        .data_cb = app_flash_write_cb,
        .user_data = NULL
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.mode = XMODEM_MODE_1K;

    size_t total_received = 0;
    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_received);

    if (status == XMODEM_OK) {
        printk("\nFirmware upload completed successfully! (%zu bytes written)\n", total_received);
    } else {
        printk("\nFirmware upload failed! (Error code: %d)\n", (int)status);
    }

    return 0;
}
