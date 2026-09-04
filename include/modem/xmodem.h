/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for XMODEM file transfer protocol engine.
 * Supports standard 128-byte block, XMODEM-CRC, and XMODEM-1K variants.
 */

#ifndef MODEM_XMODEM_H_
#define MODEM_XMODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XMODEM Control Characters */
#define XMODEM_SOH  0x01 /* Start of Header (128-byte block) */
#define XMODEM_STX  0x02 /* Start of Header (1024-byte block) */
#define XMODEM_EOT  0x04 /* End of Transmission */
#define XMODEM_ACK  0x06 /* Acknowledge */
#define XMODEM_NAK  0x15 /* Negative Acknowledge */
#define XMODEM_CAN  0x18 /* Cancel */
#define XMODEM_C    'C'  /* Request CRC-16 mode */

/* Block Payload Sizes */
#define XMODEM_BLOCK_SIZE_128  128  /* Standard block size */
#define XMODEM_BLOCK_SIZE_1024 1024 /* XMODEM-1K block size */

/**
 * @brief XMODEM Operation Status Return Codes
 */
typedef enum {
    XMODEM_OK = 0,               /**< Transfer completed successfully */
    XMODEM_ERROR = -1,           /**< Unspecified protocol error */
    XMODEM_ERROR_TIMEOUT = -2,   /**< Timed out waiting for response */
    XMODEM_ERROR_CANCEL = -3,    /**< Transfer cancelled by peer (CAN) */
    XMODEM_ERROR_SEQUENCE = -4,  /**< Block sequence number mismatch */
    XMODEM_ERROR_CRC = -5,       /**< Checksum or CRC verification failure */
    XMODEM_ERROR_IO = -6         /**< Underlying I/O or callback failure */
} xmodem_status_t;

/**
 * @brief XMODEM Operating Mode
 */
typedef enum {
    XMODEM_MODE_STANDARD = 0, /**< 128-byte block with 8-bit checksum */
    XMODEM_MODE_CRC,          /**< 128-byte block with 16-bit CCITT CRC */
    XMODEM_MODE_1K            /**< 1024-byte block with 16-bit CCITT CRC */
} xmodem_mode_t;

/**
 * @brief XMODEM Transport and Storage Callbacks
 */
typedef struct {
    /**
     * @brief Read a single byte from serial transport.
     * @param byte Output pointer for received byte.
     * @param timeout_ms Timeout in milliseconds.
     * @param user_data Opaque pointer to user context.
     * @return 0 on success, negative on error or timeout.
     */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /**
     * @brief Write bytes to serial transport.
     * @param buf Pointer to output buffer.
     * @param len Number of bytes to transmit.
     * @param user_data Opaque pointer to user context.
     * @return 0 on success, negative on I/O error.
     */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /**
     * @brief Callback for processing received block payload (Receiver) or supplying payload (Transmitter).
     * @param block_num Sequence number of block (1..255 wrapping).
     * @param buf Pointer to payload buffer.
     * @param len Size of block payload in bytes.
     * @param user_data Opaque pointer to user context.
     * @return 0 on success, negative on storage error.
     */
    int (*data_cb)(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data);

    void *user_data; /**< User context passed to callbacks */
} xmodem_callbacks_t;

/**
 * @brief XMODEM Protocol Configuration Parameters
 */
typedef struct {
    xmodem_mode_t mode;      /**< Protocol operating mode */
    uint32_t byte_timeout_ms;   /**< Inter-byte timeout in milliseconds */
    uint32_t packet_timeout_ms; /**< Packet response timeout in milliseconds */
    uint8_t max_retries;        /**< Maximum retries per packet */
} xmodem_config_t;

/**
 * @brief Initialize default XMODEM configuration.
 *
 * Configures 1K CRC mode with standard timeouts (1000ms byte, 3000ms packet, 10 retries).
 *
 * @param config Pointer to target config structure.
 */
void xmodem_config_init(xmodem_config_t *config);

/**
 * @brief Receive data via XMODEM protocol.
 *
 * Runs the XMODEM receiver state machine, negotiating CRC/checksum mode and processing incoming packets.
 *
 * @param callbacks Interface callbacks for transport and storage I/O.
 * @param config Protocol configuration (or NULL for default settings).
 * @param total_received Pointer to variable receiving total payload bytes processed.
 * @return XMODEM_OK on success or error status.
 */
xmodem_status_t xmodem_receive(const xmodem_callbacks_t *callbacks,
                               const xmodem_config_t *config,
                               size_t *total_received);

/**
 * @brief Transmit data via XMODEM protocol.
 *
 * Runs the XMODEM transmitter state machine, responding to receiver handshake and transmitting blocks.
 *
 * @param callbacks Interface callbacks for transport and payload data retrieval.
 * @param total_len Total size of file/stream in bytes (0 if unknown).
 * @param config Protocol configuration (or NULL for default settings).
 * @return XMODEM_OK on success or error status.
 */
xmodem_status_t xmodem_transmit(const xmodem_callbacks_t *callbacks,
                                size_t total_len,
                                const xmodem_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_XMODEM_H_ */
