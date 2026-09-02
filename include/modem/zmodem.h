#ifndef MODEM_ZMODEM_H_
#define MODEM_ZMODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ZMODEM Framing / Control Bytes */
#define ZPAD        '*'     /* Pad character */
#define ZDLE        0x18    /* Control sequence prefix (CAN) */
#define ZDLEE       (ZDLE^0x40)
#define ZBIN        'A'     /* Binary frame indicator (16-bit CRC) */
#define ZHEX        'B'     /* Hex frame indicator */
#define ZBIN32      'C'     /* Binary frame indicator with 32-bit CRC */

/* ZMODEM Frame Types */
#define ZRQINIT     0       /* Request receive init */
#define ZRINIT      1       /* Receive init */
#define ZSINIT      2       /* Send init sequence */
#define ZACK        3       /* ACK */
#define ZFILE       4       /* File name / header */
#define ZSKIP       5       /* Skip file */
#define ZNAK        6       /* Last packet was bad */
#define ZABORT      7       /* Abort batch */
#define ZFIN        8       /* Finish session */
#define ZRPOS       9       /* Resume file position */
#define ZDATA       10      /* Data subpacket header */
#define ZEOF        11      /* End of file */
#define ZFERR       12      /* Fatal I/O error */
#define ZCRC        13      /* Request file CRC */
#define ZCHALLENGE  14      /* Receiver challenge */
#define ZCOMPL      15      /* Request complete */
#define CANCEL      0x18    /* Cancel sequence */

/* ZDLE Data Subpacket Enders */
#define ZCRCE       'h'     /* CRC next, frame ends, header follows */
#define ZCRCG       'i'     /* CRC next, frame continues non-stop */
#define ZCRCQ       'j'     /* CRC next, send ZACK frame */
#define ZCRCW       'k'     /* CRC next, send ZACK, end of frame */

typedef enum {
    ZMODEM_OK = 0,
    ZMODEM_ERROR = -1,
    ZMODEM_ERROR_TIMEOUT = -2,
    ZMODEM_ERROR_CANCEL = -3,
    ZMODEM_ERROR_CRC = -4,
    ZMODEM_ERROR_IO = -5
} zmodem_status_t;

typedef struct {
    char filename[256];
    size_t size;
} zmodem_file_info_t;

typedef struct {
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);
    int (*on_file_start)(const zmodem_file_info_t *info, void *user_data);
    int (*on_data)(const uint8_t *buf, size_t len, size_t offset, void *user_data);
    void (*on_file_end)(const zmodem_file_info_t *info, zmodem_status_t status, void *user_data);
    void *user_data;
} zmodem_rx_callbacks_t;

typedef struct {
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);
    int (*get_file_info)(size_t file_index, zmodem_file_info_t *info, void *user_data);
    int (*read_data)(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data);
    void *user_data;
} zmodem_tx_callbacks_t;

/**
 * @brief Receive batch of files via ZMODEM protocol.
 * @param callbacks Callbacks for receiver.
 * @return ZMODEM_OK on success or error status.
 */
zmodem_status_t zmodem_receive(const zmodem_rx_callbacks_t *callbacks);

/**
 * @brief Transmit batch of files via ZMODEM protocol.
 * @param callbacks Callbacks for transmitter.
 * @return ZMODEM_OK on success or error status.
 */
zmodem_status_t zmodem_transmit(const zmodem_tx_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZMODEM_H_ */
