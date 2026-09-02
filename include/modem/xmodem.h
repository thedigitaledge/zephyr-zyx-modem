#ifndef MODEM_XMODEM_H_
#define MODEM_XMODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Control characters */
#define XMODEM_SOH  0x01
#define XMODEM_STX  0x02
#define XMODEM_EOT  0x04
#define XMODEM_ACK  0x06
#define XMODEM_NAK  0x15
#define XMODEM_CAN  0x18
#define XMODEM_C    'C'

/* Packet sizes */
#define XMODEM_BLOCK_SIZE_128  128
#define XMODEM_BLOCK_SIZE_1024 1024

/* Return codes */
typedef enum {
    XMODEM_OK = 0,
    XMODEM_ERROR = -1,
    XMODEM_ERROR_TIMEOUT = -2,
    XMODEM_ERROR_CANCEL = -3,
    XMODEM_ERROR_SEQUENCE = -4,
    XMODEM_ERROR_CRC = -5,
    XMODEM_ERROR_IO = -6
} xmodem_status_t;

/* Mode selection */
typedef enum {
    XMODEM_MODE_STANDARD = 0, /* 128-byte block with Checksum */
    XMODEM_MODE_CRC,          /* 128-byte block with CRC16 */
    XMODEM_MODE_1K            /* 1024-byte block with CRC16 */
} xmodem_mode_t;

/* Transport & Storage Callbacks */
typedef struct {
    /**
     * @brief Read a single byte from the serial transport.
     * @param byte Output pointer for the read byte.
     * @param timeout_ms Timeout in milliseconds.
     * @param user_data User context pointer.
     * @return 0 on success, negative on timeout or error.
     */
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);

    /**
     * @brief Write bytes to the serial transport.
     * @param buf Pointer to buffer to write.
     * @param len Number of bytes to write.
     * @param user_data User context pointer.
     * @return 0 on success, negative on error.
     */
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);

    /**
     * @brief Callback invoked when a payload block is received (Receiver) or requested (Transmitter).
     * @param block_num Block number (1..255, wrapping).
     * @param buf Payload buffer (128 or 1024 bytes).
     * @param len Size of block payload.
     * @param user_data User context pointer.
     * @return 0 on success, negative on error.
     */
    int (*data_cb)(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data);

    void *user_data;
} xmodem_callbacks_t;

/* Configuration options */
typedef struct {
    xmodem_mode_t mode;
    uint32_t byte_timeout_ms;
    uint32_t packet_timeout_ms;
    uint8_t max_retries;
} xmodem_config_t;

/**
 * @brief Initialize default XMODEM configuration.
 * @param config Pointer to config structure.
 */
void xmodem_config_init(xmodem_config_t *config);

/**
 * @brief Receive data via XMODEM protocol.
 * @param callbacks I/O and data callback interface.
 * @param config Configuration (or NULL for default CRC/1K mode).
 * @param total_received Pointer to variable receiving total payload bytes received.
 * @return XMODEM_OK on success, or error code.
 */
xmodem_status_t xmodem_receive(const xmodem_callbacks_t *callbacks,
                               const xmodem_config_t *config,
                               size_t *total_received);

/**
 * @brief Transmit data via XMODEM protocol.
 * @param callbacks I/O and data callback interface (data_cb is called to fill block buffer).
 * @param total_len Total size of data to transmit (0 if unknown).
 * @param config Configuration (or NULL for default).
 * @return XMODEM_OK on success, or error code.
 */
xmodem_status_t xmodem_transmit(const xmodem_callbacks_t *callbacks,
                                size_t total_len,
                                const xmodem_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_XMODEM_H_ */
