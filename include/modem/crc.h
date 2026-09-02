#ifndef MODEM_CRC_H_
#define MODEM_CRC_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate simple 8-bit arithmetic checksum.
 *
 * Used in basic XMODEM checksum mode.
 *
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 8-bit checksum (sum of bytes modulo 256).
 */
uint8_t modem_checksum8(const uint8_t *buf, size_t len);

/**
 * @brief Calculate 16-bit CRC (CCITT polynomial 0x1021, init 0x0000).
 *
 * Used in XMODEM-CRC, YMODEM, and ZMODEM16.
 *
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 16-bit CRC value.
 */
uint16_t modem_crc16(const uint8_t *buf, size_t len);

/**
 * @brief Update ongoing 16-bit CRC (CCITT polynomial 0x1021).
 *
 * @param crc Previous CRC value (start with 0x0000).
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return Updated 16-bit CRC value.
 */
uint16_t modem_crc16_update(uint16_t crc, const uint8_t *buf, size_t len);

/**
 * @brief Calculate 32-bit CRC (IEEE 802.3 polynomial, init 0xFFFFFFFF, xorout 0xFFFFFFFF).
 *
 * Used in ZMODEM32.
 *
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 32-bit CRC value.
 */
uint32_t modem_crc32(const uint8_t *buf, size_t len);

/**
 * @brief Update ongoing 32-bit CRC (IEEE 802.3).
 *
 * @param crc Previous raw CRC value (start with 0xFFFFFFFF).
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return Updated raw CRC value (before final XOR with 0xFFFFFFFF).
 */
uint32_t modem_crc32_update(uint32_t crc, const uint8_t *buf, size_t len);

/**
 * @brief Finalize 32-bit CRC update.
 *
 * @param crc Raw accumulated CRC value from modem_crc32_update.
 * @return Final 32-bit CRC value (XORed with 0xFFFFFFFF).
 */
static inline uint32_t modem_crc32_finalize(uint32_t crc) {
    return crc ^ 0xFFFFFFFF;
}

#ifdef __cplusplus
}
#endif

#endif /* MODEM_CRC_H_ */
