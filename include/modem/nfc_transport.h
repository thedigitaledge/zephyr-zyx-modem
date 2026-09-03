/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Concrete NFC (Near Field Communication) NDEF Modem Transport Adapter for Zephyr OS.
 * Built strictly for Nordic Semiconductor nRF Connect SDK NFC Subsystem and Type 4 Tag (T4T) library.
 */

#ifndef MODEM_NFC_TRANSPORT_H_
#define MODEM_NFC_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(CONFIG_NFC_T4T_NRFXLIB) || defined(CONFIG_NFC_NDEF_MSG)
#include <nfc/t4t/emulation.h>
#include <nfc/t4t/msgtag.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/record.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_TRANSPORT_NDEF_MAX_SIZE 512

/**
 * @brief Modem NFC T4T Event Representation
 */
typedef enum {
    NFC_MODEM_EVENT_FIELD_ON = 0,   /**< RF field detected by NFC controller */
    NFC_MODEM_EVENT_FIELD_OFF,      /**< RF field loss detected */
    NFC_MODEM_EVENT_NDEF_READ,      /**< NDEF message read by external poller */
    NFC_MODEM_EVENT_NDEF_UPDATED,   /**< New NDEF record written by external poller */
    NFC_MODEM_EVENT_DATA_IND        /**< Direct ISODEP raw data indication */
} nfc_modem_event_t;

/**
 * @brief NFC Transport Diagnostic Statistics
 */
typedef struct {
    size_t rx_bytes;            /**< Total received byte count */
    size_t tx_bytes;            /**< Total transmitted byte count */
    uint32_t ndef_records_rx;   /**< Total NDEF records received and decoded */
    uint32_t ndef_records_tx;   /**< Total NDEF records encoded and published */
    uint32_t field_loss_count;   /**< Total RF field loss events */
    uint32_t overflow_count;     /**< RX ring buffer overflow count */
    uint32_t t4t_events_handled; /**< Total T4T event callbacks processed */
} nfc_transport_stats_t;

/**
 * @brief NFC Transport Configuration Parameters
 */
typedef struct {
    size_t rx_ring_size;        /**< Internal RX ring buffer capacity in bytes */
    size_t tx_ring_size;        /**< Internal TX ring buffer capacity in bytes */
    size_t rx_buffer_size;      /**< Alias for rx_ring_size */
    size_t tx_buffer_size;      /**< Alias for tx_ring_size */
    uint32_t field_timeout_ms;  /**< Timeout in ms for RF field presence loss */
    bool auto_ndef_framing;     /**< Automatically wrap TX payloads in NDEF media records */
    int (*raw_tx_cb)(const uint8_t *ndef_buf, size_t len, void *user_data); /**< Physical NFC driver TX callback */
    void *user_data;            /**< User context pointer */
} nfc_transport_config_t;

/**
 * @brief Initialize concrete nRF Connect SDK NFC T4T modem transport adapter.
 * @param config Configuration parameters (NULL for default 1024-byte ring buffers).
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_init(const nfc_transport_config_t *config);

/**
 * @brief Start NFC Type 4 Tag (T4T) emulation using nRF Connect SDK nfc_t4t_emulation_start().
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_start(void);

/**
 * @brief Stop NFC Type 4 Tag (T4T) emulation using nRF Connect SDK nfc_t4t_emulation_stop().
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_stop(void);

/**
 * @brief Start NFC T4T emulation mode (alias for nfc_transport_start).
 */
int nfc_transport_start_t4t_emulation(void);

/**
 * @brief Stop NFC T4T emulation mode (alias for nfc_transport_stop).
 */
int nfc_transport_stop_t4t_emulation(void);

/**
 * @brief Nordic nRF Connect SDK NFC T4T Event Handler Callback (4 arguments matching nfc_t4t_callback_t).
 * @param event T4T event enum value.
 * @param data Data buffer associated with event (or NULL).
 * @param data_len Length of data buffer.
 * @param context User context pointer.
 */
void nfc_transport_t4t_event_handler(int event, const uint8_t *data, size_t data_len, void *context);

/**
 * @brief Read byte from NFC NDEF transport stream with timeout polling.
 * @param byte Output pointer for received byte.
 * @param timeout_ms Timeout in milliseconds.
 * @param user_data Opaque user context.
 * @return 0 on success, -1 on timeout/error, -2 on RF field loss cancellation.
 */
int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);

/**
 * @brief Write bytes to NFC NDEF transport stream for T4T NDEF tag publication.
 * @param buf Data buffer to send.
 * @param len Length of data in bytes.
 * @param user_data Opaque user context.
 * @return 0 on success, negative error code on failure.
 */
int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief Process incoming raw NDEF record payload or serial bytes from nRF Connect SDK NFC reader.
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
 * @brief Set RF field presence status.
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
 * @brief Helper function to encode raw payload into NDEF Media Record using nRF Connect SDK API formats.
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
