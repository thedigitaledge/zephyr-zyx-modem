/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Concrete NFC (Near Field Communication) Modem Transport Adapter for Zephyr OS.
 * Built strictly on top of Nordic Semiconductor nRF Connect SDK NFC Subsystem and T4T libraries.
 */

#include <modem/nfc_transport.h>
#include <zephyr/kernel.h>
#include <string.h>

#define NFC_RING_BUF_SIZE 1024
#define NFC_MIME_TYPE "application/x-modem"
#define NFC_MIME_TYPE_LEN 19

static uint8_t nfc_rx_buf[NFC_RING_BUF_SIZE];
static size_t nfc_rx_head = 0;
static size_t nfc_rx_tail = 0;
static size_t nfc_rx_count = 0;

static uint8_t nfc_tx_buf[NFC_RING_BUF_SIZE];
static size_t nfc_tx_head = 0;
static size_t nfc_tx_tail = 0;
static size_t nfc_tx_count = 0;

static bool nfc_field_active = false;
static bool nfc_emulation_running = false;
static nfc_transport_config_t g_nfc_config = {0};
static nfc_transport_stats_t g_nfc_stats = {0};

int nfc_transport_init(const nfc_transport_config_t *config)
{
    if (config) {
        g_nfc_config = *config;
    } else {
        memset(&g_nfc_config, 0, sizeof(g_nfc_config));
        g_nfc_config.rx_ring_size = NFC_RING_BUF_SIZE;
        g_nfc_config.tx_ring_size = NFC_RING_BUF_SIZE;
        g_nfc_config.field_timeout_ms = 1000;
        g_nfc_config.auto_ndef_framing = true;
    }

    nfc_rx_head = 0;
    nfc_rx_tail = 0;
    nfc_rx_count = 0;

    nfc_tx_head = 0;
    nfc_tx_tail = 0;
    nfc_tx_count = 0;

    nfc_field_active = false;
    nfc_emulation_running = false;
    memset(&g_nfc_stats, 0, sizeof(g_nfc_stats));

    return 0;
}

int nfc_transport_start(void)
{
#if defined(CONFIG_NFC_T4T_NRFXLIB)
    int err = nfc_t4t_setup((nfc_t4t_callback_t)nfc_transport_t4t_event_handler, NULL);
    if (err) {
        return err;
    }
    err = nfc_t4t_emulation_start();
    if (err) {
        return err;
    }
#endif
    nfc_field_active = true;
    nfc_emulation_running = true;
    return 0;
}

int nfc_transport_stop(void)
{
#if defined(CONFIG_NFC_T4T_NRFXLIB)
    nfc_t4t_emulation_stop();
#endif
    nfc_field_active = false;
    nfc_emulation_running = false;
    return 0;
}

int nfc_transport_start_t4t_emulation(void)
{
    return nfc_transport_start();
}

int nfc_transport_stop_t4t_emulation(void)
{
    return nfc_transport_stop();
}

void nfc_transport_t4t_event_handler(int event, const uint8_t *data, size_t data_len, void *context)
{
    (void)context;
    g_nfc_stats.t4t_events_handled++;

    /* Process event type matching both nRF Connect SDK enum and test events */
    if (event == 0 || event == NFC_MODEM_EVENT_FIELD_ON) {
        nfc_transport_set_field_active(true);
    } else if (event == 1 || event == NFC_MODEM_EVENT_FIELD_OFF) {
        nfc_transport_set_field_active(false);
    } else if (event == 3 || event == 4 || event == NFC_MODEM_EVENT_NDEF_UPDATED || event == NFC_MODEM_EVENT_DATA_IND) {
        if (data && data_len > 0) {
            nfc_transport_rx_callback(data, data_len);
        }
    }
}

int nfc_transport_encode_ndef_record(const uint8_t *payload, size_t payload_len,
                                     uint8_t *ndef_buf, size_t buf_capacity,
                                     size_t *ndef_len)
{
    if (!payload || !ndef_buf || !ndef_len || payload_len > 255) {
        return -1;
    }

    size_t required = 3 + NFC_MIME_TYPE_LEN + payload_len;
    if (buf_capacity < required) {
        return -1;
    }

    /* NDEF Header: MB=1, ME=1, CF=0, SR=1, IL=0, TNF=0x02 (MIME Media) -> 0xD2 */
    ndef_buf[0] = 0xD2;
    ndef_buf[1] = (uint8_t)NFC_MIME_TYPE_LEN;
    ndef_buf[2] = (uint8_t)payload_len;

    memcpy(&ndef_buf[3], NFC_MIME_TYPE, NFC_MIME_TYPE_LEN);
    memcpy(&ndef_buf[3 + NFC_MIME_TYPE_LEN], payload, payload_len);

    *ndef_len = required;
    g_nfc_stats.ndef_records_tx++;
    return 0;
}

void nfc_transport_rx_callback(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return;
    }

    const uint8_t *payload_ptr = data;
    size_t payload_len = len;

    /* Detect NDEF Record Header (MB=1, ME=1, TNF=MIME -> 0xD2) */
    if (len > 3 + NFC_MIME_TYPE_LEN && data[0] == 0xD2) {
        uint8_t type_len = data[1];
        uint8_t pay_len = data[2];

        if (type_len == NFC_MIME_TYPE_LEN && (size_t)(3 + type_len + pay_len) <= len) {
            if (memcmp(&data[3], NFC_MIME_TYPE, NFC_MIME_TYPE_LEN) == 0) {
                payload_ptr = &data[3 + NFC_MIME_TYPE_LEN];
                payload_len = pay_len;
                g_nfc_stats.ndef_records_rx++;
            }
        }
    }

    for (size_t i = 0; i < payload_len; i++) {
        if (nfc_rx_count < sizeof(nfc_rx_buf)) {
            nfc_rx_buf[nfc_rx_head] = payload_ptr[i];
            nfc_rx_head = (nfc_rx_head + 1) % sizeof(nfc_rx_buf);
            nfc_rx_count++;
            g_nfc_stats.rx_bytes++;
        } else {
            g_nfc_stats.overflow_count++;
        }
    }
}

int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)user_data;
    if (!byte) {
        return -1;
    }

    int64_t start = k_uptime_get();
    uint32_t wait_limit = (timeout_ms == 0) ? 1 : timeout_ms;

    while (k_uptime_get() - start <= wait_limit) {
        if (!nfc_field_active) {
            return -2; /* RF field loss signal */
        }
        if (nfc_rx_count > 0) {
            *byte = nfc_rx_buf[nfc_rx_tail];
            nfc_rx_tail = (nfc_rx_tail + 1) % sizeof(nfc_rx_buf);
            nfc_rx_count--;
            return 0;
        }
        k_msleep(5);
    }

    return -1;
}

int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    if (!buf || len == 0) {
        return -1;
    }
    if (!nfc_field_active) {
        return -2;
    }

    for (size_t i = 0; i < len; i++) {
        if (nfc_tx_count < sizeof(nfc_tx_buf)) {
            nfc_tx_buf[nfc_tx_head] = buf[i];
            nfc_tx_head = (nfc_tx_head + 1) % sizeof(nfc_tx_buf);
            nfc_tx_count++;
            g_nfc_stats.tx_bytes++;
        }
    }

    if (g_nfc_config.auto_ndef_framing && g_nfc_config.raw_tx_cb) {
        uint8_t ndef_msg[NFC_TRANSPORT_NDEF_MAX_SIZE];
        size_t ndef_len = 0;
        if (nfc_transport_flush_tx_ndef(ndef_msg, sizeof(ndef_msg), &ndef_len) == 0) {
            g_nfc_config.raw_tx_cb(ndef_msg, ndef_len, g_nfc_config.user_data);
        }
    }

    return 0;
}

int nfc_transport_flush_tx_ndef(uint8_t *out_buf, size_t out_capacity, size_t *out_len)
{
    if (!out_buf || !out_len || nfc_tx_count == 0) {
        return -1;
    }

    uint8_t temp_payload[255];
    size_t extract_len = (nfc_tx_count < sizeof(temp_payload)) ? nfc_tx_count : sizeof(temp_payload);

    for (size_t i = 0; i < extract_len; i++) {
        temp_payload[i] = nfc_tx_buf[nfc_tx_tail];
        nfc_tx_tail = (nfc_tx_tail + 1) % sizeof(nfc_tx_buf);
        nfc_tx_count--;
    }

    return nfc_transport_encode_ndef_record(temp_payload, extract_len, out_buf, out_capacity, out_len);
}

void nfc_transport_set_field_active(bool active)
{
    if (nfc_field_active && !active) {
        g_nfc_stats.field_loss_count++;
    }
    nfc_field_active = active;
}

bool nfc_transport_is_active(void)
{
    return nfc_field_active;
}

void nfc_transport_get_stats(nfc_transport_stats_t *stats)
{
    if (stats) {
        *stats = g_nfc_stats;
    }
}

void nfc_transport_reset_stats(void)
{
    memset(&g_nfc_stats, 0, sizeof(g_nfc_stats));
}
