/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for NFC (Near Field Communication) Modem Transport Adapter for Zephyr OS.
 */

#ifndef MODEM_NFC_TRANSPORT_H_
#define MODEM_NFC_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC Transport Configuration
 */
typedef struct {
    size_t rx_buffer_size;  /**< Internal NDEF RX buffer size */
    uint32_t field_timeout_ms; /**< NFC field loss timeout */
} nfc_transport_config_t;

/**
 * @brief Initialize NFC data communications transport adapter.
 * @param config NFC configuration parameters.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_init(const nfc_transport_config_t *config);

/**
 * @brief Read byte from NFC NDEF buffer stream.
 * @param byte Output pointer for received byte.
 * @param timeout_ms Timeout in milliseconds.
 * @param user_data User context pointer.
 * @return 0 on success, -1 on timeout, -2 on cancellation.
 */
int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);

/**
 * @brief Write bytes over NFC tag/field emulator stream.
 * @param buf Data buffer to send.
 * @param len Length of data in bytes.
 * @param user_data User context pointer.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief Callback triggered on incoming NFC NDEF payload record.
 * @param data Received NDEF payload.
 * @param len Received length.
 */
void nfc_transport_rx_callback(const uint8_t *data, size_t len);

/**
 * @brief Check if NFC field/tag presence is active.
 * @return true if field detected, false otherwise.
 */
bool nfc_transport_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_NFC_TRANSPORT_H_ */
