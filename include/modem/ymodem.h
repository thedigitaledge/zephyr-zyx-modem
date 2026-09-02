/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Header file for YMODEM batch file transfer protocol engine.
 * Supports Block 0 file metadata parsing, 128-byte and 1024-byte payloads, and batch transfers.
 */

#ifndef MODEM_YMODEM_H_
#define MODEM_YMODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief YMODEM Protocol Status Codes
 */
typedef enum {
    YMODEM_OK = 0,               /**< Transfer completed successfully */
    YMODEM_ERROR = -1,           /**< General YMODEM protocol error */
    YMODEM_ERROR_TIMEOUT = -2,   /**< Timed out waiting for response */
    YMODEM_ERROR_CANCEL = -3,    /**< Transfer cancelled by peer */
    YMODEM_ERROR_SEQUENCE = -4,  /**< Block sequence error */
    YMODEM_ERROR_CRC = -5,       /**< CRC verification failed */
    YMODEM_ERROR_IO = -6         /**< Storage or transport I/O error */
} ymodem_status_t;

/**
 * @brief YMODEM File Information Structure
 */
typedef struct {
    char filename[256]; /**< Null-terminated filename */
    size_t size;        /**< File length in bytes */
} ymodem_file_info_t;

/**
 * @brief YMODEM Receiver Callback Functions
 */
typedef struct {
    /** Read a byte from serial transport */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /** Write buffer to serial transport */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /** Invoked when file header (Block 0) is received */
    int (*on_file_start)(const ymodem_file_info_t *info, void *user_data);

    /** Invoked when file data chunk is received */
    int (*on_data)(const uint8_t *buf, size_t len, size_t offset, void *user_data);

    /** Invoked when file transfer ends */
    void (*on_file_end)(const ymodem_file_info_t *info, ymodem_status_t status, void *user_data);

    void *user_data; /**< User context pointer */
} ymodem_rx_callbacks_t;

/**
 * @brief YMODEM Transmitter Callback Functions
 */
typedef struct {
    /** Read a byte from serial transport */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /** Write buffer to serial transport */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /** Fetch file metadata for file index `file_index` (0-based). Return 0 if available, < 0 when batch ends */
    int (*get_file_info)(size_t file_index, ymodem_file_info_t *info, void *user_data);

    /** Read chunk of data from file being transmitted */
    int (*read_data)(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data);

    void *user_data; /**< User context pointer */
} ymodem_tx_callbacks_t;

/**
 * @brief Receive a batch of files using YMODEM protocol.
 * @param callbacks Receiver callbacks interface.
 * @return YMODEM_OK on successful batch completion or error code.
 */
ymodem_status_t ymodem_receive(const ymodem_rx_callbacks_t *callbacks);

/**
 * @brief Transmit a batch of files using YMODEM protocol.
 * @param callbacks Transmitter callbacks interface.
 * @return YMODEM_OK on successful batch completion or error code.
 */
ymodem_status_t ymodem_transmit(const ymodem_tx_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_YMODEM_H_ */
