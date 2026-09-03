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
#include <string.h>
#include <errno.h>

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

#if defined(CONFIG_MODEM_NFC)

int nfc_transport_init(const nfc_transport_config_t *config);
int nfc_transport_start(void);
int nfc_transport_stop(void);
int nfc_transport_start_t4t_emulation(void);
int nfc_transport_stop_t4t_emulation(void);
void nfc_transport_t4t_event_handler(int event, const uint8_t *data, size_t data_len, void *context);
int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);
int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);
void nfc_transport_rx_callback(const uint8_t *data, size_t len);
int nfc_transport_flush_tx_ndef(uint8_t *out_buf, size_t out_capacity, size_t *out_len);
void nfc_transport_set_field_active(bool active);
bool nfc_transport_is_active(void);
void nfc_transport_get_stats(nfc_transport_stats_t *stats);
void nfc_transport_reset_stats(void);
int nfc_transport_encode_ndef_record(const uint8_t *payload, size_t payload_len,
                                     uint8_t *ndef_buf, size_t buf_capacity,
                                     size_t *ndef_len);

#else

static inline int nfc_transport_init(const nfc_transport_config_t *config) { (void)config; return -ENOTSUP; }
static inline int nfc_transport_start(void) { return -ENOTSUP; }
static inline int nfc_transport_stop(void) { return -ENOTSUP; }
static inline int nfc_transport_start_t4t_emulation(void) { return -ENOTSUP; }
static inline int nfc_transport_stop_t4t_emulation(void) { return -ENOTSUP; }
static inline void nfc_transport_t4t_event_handler(int event, const uint8_t *data, size_t data_len, void *context) { (void)event; (void)data; (void)data_len; (void)context; }
static inline int nfc_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data) { (void)byte; (void)timeout_ms; (void)user_data; return -ENOTSUP; }
static inline int nfc_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data) { (void)buf; (void)len; (void)user_data; return -ENOTSUP; }
static inline void nfc_transport_rx_callback(const uint8_t *data, size_t len) { (void)data; (void)len; }
static inline int nfc_transport_flush_tx_ndef(uint8_t *out_buf, size_t out_capacity, size_t *out_len) { (void)out_buf; (void)out_capacity; (void)out_len; return -ENOTSUP; }
static inline void nfc_transport_set_field_active(bool active) { (void)active; }
static inline bool nfc_transport_is_active(void) { return false; }
static inline void nfc_transport_get_stats(nfc_transport_stats_t *stats) { if (stats) { memset(stats, 0, sizeof(*stats)); } }
static inline void nfc_transport_reset_stats(void) {}
static inline int nfc_transport_encode_ndef_record(const uint8_t *payload, size_t payload_len,
                                            uint8_t *ndef_buf, size_t buf_capacity,
                                            size_t *ndef_len) { (void)payload; (void)payload_len; (void)ndef_buf; (void)buf_capacity; (void)ndef_len; return -ENOTSUP; }

#endif /* CONFIG_MODEM_NFC */

#ifdef __cplusplus
}
#endif

#endif /* MODEM_NFC_TRANSPORT_H_ */
