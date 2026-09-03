/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for NFC (Near Field Communication) NDEF Modem Transport Adapter for Zephyr OS.
 * Provides ring-buffered data streams, NDEF Type-4 record encapsulation, field loss
 * detection timeouts, and transmission/reception callbacks.
 */

#ifndef MODEM_NFC_TRANSPORT_H_
#define MODEM_NFC_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_MAX_NDEF_PAYLOAD_SIZE 512

/**
 * @brief NFC Transport Diagnostic Statistics
 */
typedef struct {
    size_t rx_bytes;       /**< Total received byte count */
    size_t tx_bytes;       /**< Total transmitted byte count */
    uint32_t ndef_records_rx; /**< Total NDEF records decoded */
    uint32_t ndef_records_tx; /**< Total NDEF records encoded */
    uint32_t field_loss_count; /**< Total RF field loss events detected */
    uint32_t overflow_count;   /**< RX buffer overflow drop count */
} nfc_transport_stats_t;

/**
 * @brief NFC Transport Configuration Parameters
 */
typedef struct {
    size_t rx_buffer_size;       /**< Internal RX ring buffer capacity in bytes */
    size_t tx_buffer_size;       /**< Internal TX ring buffer capacity in bytes */
    uint32_t field_timeout_ms;    /**< RF field loss timeout in milliseconds */
    bool auto_ndef_framing;      /**< Automatically wrap TX payloads in NDEF media records */
    int (*raw_tx_cb)(const uint8_t *ndef_buf, size_t len, void *user_data); /**< Physical NFC driver TX callback */
    void *user_data;             /**< User context passed to callbacks */
} nfc_transport_config_t;

/**
 * @brief Initialize NFC data communications transport adapter.
 * @param config Pointer to NFC configuration parameters (NULL for default settings).
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_init(const nfc_transport_config_t *config);

/**
 * @brief Read byte from NFC NDEF RX stream with timeout polling.
 * @param byte Output pointer for received byte.
 * @param timeout_ms Timeout in milliseconds.
 * @param user_data Opaque user context.
 * @return 0 on success, -1 on timeout/error, -2 on cancellation.
 */
int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);

/**
 * @brief Write bytes over NFC tag/field emulator output stream.
 * @param buf Data buffer to send.
 * @param len Length of data in bytes.
 * @param user_data Opaque user context.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief Process incoming raw NDEF record payload or serial bytes from NFC reader.
 * @param data Received NDEF payload or byte buffer.
 * @param len Received data length.
 */
void nfc_transport_rx_callback(const uint8_t *data, size_t len);

/**
 * @brief Flush pending TX ring buffer bytes as an NDEF MIME payload record.
 * @param out_buf Output buffer to store constructed NDEF message.
 * @param out_capacity Capacity of output buffer.
 * @param out_len Pointer receiving length of constructed NDEF message.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_flush_tx_ndef(uint8_t *out_buf, size_t out_capacity, size_t *out_len);

/**
 * @brief Set RF field presence status (called by physical NFC controller ISR/driver).
 * @param active true if RF field is active/present, false on field loss.
 */
void nfc_transport_set_field_active(bool active);

/**
 * @brief Check if NFC RF field presence is currently active.
 * @return true if field detected and active, false otherwise.
 */
bool nfc_transport_is_active(void);

/**
 * @brief Get cumulative NFC transport diagnostic statistics.
 * @param stats Output pointer receiving statistics.
 */
void nfc_transport_get_stats(nfc_transport_stats_t *stats);

/**
 * @brief Reset cumulative NFC transport statistics counters.
 */
void nfc_transport_reset_stats(void);

/**
 * @brief Helper function to encode raw payload into NDEF Media Record (mime: "application/x-modem").
 * @param payload Raw serial payload data.
 * @param payload_len Payload length in bytes.
 * @param ndef_buf Output buffer receiving encoded NDEF frame.
 * @param buf_capacity Capacity of output buffer.
 * @param ndef_len Output pointer receiving total encoded NDEF record length.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_encode_ndef_record(const uint8_t *payload, size_t payload_len,
                                     uint8_t *ndef_buf, size_t buf_capacity,
                                     size_t *ndef_len);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_NFC_TRANSPORT_H_ */
