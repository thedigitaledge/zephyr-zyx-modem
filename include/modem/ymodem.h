#ifndef MODEM_YMODEM_H_
#define MODEM_YMODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    YMODEM_OK = 0,
    YMODEM_ERROR = -1,
    YMODEM_ERROR_TIMEOUT = -2,
    YMODEM_ERROR_CANCEL = -3,
    YMODEM_ERROR_SEQUENCE = -4,
    YMODEM_ERROR_CRC = -5,
    YMODEM_ERROR_IO = -6
} ymodem_status_t;

typedef struct {
    char filename[256];
    size_t size;
} ymodem_file_info_t;

typedef struct {
    /** Read a single byte from transport */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /** Write bytes to transport */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /** Called when a new file header (Block 0) is received */
    int (*on_file_start)(const ymodem_file_info_t *info, void *user_data);

    /** Called when data block is received */
    int (*on_data)(const uint8_t *buf, size_t len, size_t offset, void *user_data);

    /** Called when file transfer finishes */
    void (*on_file_end)(const ymodem_file_info_t *info, ymodem_status_t status, void *user_data);

    void *user_data;
} ymodem_rx_callbacks_t;

typedef struct {
    /** Read a single byte from transport */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /** Write bytes to transport */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /** Get file info for file index `file_index` (0-based). Return 0 if file available, < 0 if no more files */
    int (*get_file_info)(size_t file_index, ymodem_file_info_t *info, void *user_data);

    /** Read chunk of data from file being transmitted */
    int (*read_data)(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data);

    void *user_data;
} ymodem_tx_callbacks_t;

/**
 * @brief Receive batch of files via YMODEM protocol.
 * @param callbacks Callbacks for receiver.
 * @return YMODEM_OK on success or error status.
 */
ymodem_status_t ymodem_receive(const ymodem_rx_callbacks_t *callbacks);

/**
 * @brief Transmit batch of files via YMODEM protocol.
 * @param callbacks Callbacks for transmitter.
 * @return YMODEM_OK on success or error status.
 */
ymodem_status_t ymodem_transmit(const ymodem_tx_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_YMODEM_H_ */
