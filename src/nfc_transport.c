/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of NFC Modem Transport Adapter for Zephyr OS.
 */

#include <modem/nfc_transport.h>

#define NFC_DEFAULT_BUF_SIZE 256

static uint8_t nfc_rx_buf[NFC_DEFAULT_BUF_SIZE];
static size_t nfc_rx_head = 0;
static size_t nfc_rx_tail = 0;
static size_t nfc_rx_count = 0;
static bool nfc_field_active = false;

int nfc_transport_init(const nfc_transport_config_t *config)
{
    (void)config;
    nfc_rx_head = 0;
    nfc_rx_tail = 0;
    nfc_rx_count = 0;
    nfc_field_active = true;
    return 0;
}

void nfc_transport_rx_callback(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return;
    }
    for (size_t i = 0; i < len; i++) {
        if (nfc_rx_count < sizeof(nfc_rx_buf)) {
            nfc_rx_buf[nfc_rx_head] = data[i];
            nfc_rx_head = (nfc_rx_head + 1) % sizeof(nfc_rx_buf);
            nfc_rx_count++;
        }
    }
}

int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
    if (!byte) {
        return -1;
    }
    if (nfc_rx_count == 0) {
        return -1;
    }
    *byte = nfc_rx_buf[nfc_rx_tail];
    nfc_rx_tail = (nfc_rx_tail + 1) % sizeof(nfc_rx_buf);
    nfc_rx_count--;
    return 0;
}

int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    if (!buf || len == 0) {
        return -1;
    }
    /* Send payload over NFC tag emulation NDEF message */
    return 0;
}

bool nfc_transport_is_active(void)
{
    return nfc_field_active;
}
