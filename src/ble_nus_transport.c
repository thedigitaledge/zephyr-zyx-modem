/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of BLE Nordic UART Service (NUS) transport adapter.
 */

#include <modem/ble_nus_transport.h>

#define BLE_NUS_DEFAULT_BUF_SIZE 512

static uint8_t nus_rx_buf[BLE_NUS_DEFAULT_BUF_SIZE];
static size_t nus_rx_head = 0;
static size_t nus_rx_tail = 0;
static size_t nus_rx_count = 0;
static bool nus_connected = false;

int ble_nus_transport_init(const ble_nus_transport_config_t *config)
{
    (void)config;
    nus_rx_head = 0;
    nus_rx_tail = 0;
    nus_rx_count = 0;
    nus_connected = true;
    return 0;
}

void ble_nus_transport_rx_callback(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        if (nus_rx_count < sizeof(nus_rx_buf)) {
            nus_rx_buf[nus_rx_head] = data[i];
            nus_rx_head = (nus_rx_head + 1) % sizeof(nus_rx_buf);
            nus_rx_count++;
        }
    }
}

int ble_nus_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
    if (!byte) {
        return -1;
    }
    if (nus_rx_count == 0) {
        return -1;
    }
    *byte = nus_rx_buf[nus_rx_tail];
    nus_rx_tail = (nus_rx_tail + 1) % sizeof(nus_rx_buf);
    nus_rx_count--;
    return 0;
}

int ble_nus_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    if (!buf || len == 0) {
        return -1;
    }
    /* Simulate/Process writing over BLE NUS characteristic */
    return 0;
}

bool ble_nus_transport_is_connected(void)
{
    return nus_connected;
}
