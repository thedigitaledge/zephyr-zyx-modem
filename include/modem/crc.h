#ifndef MODEM_CRC_H_
#define MODEM_CRC_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/crc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate simple 8-bit arithmetic checksum.
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 8-bit checksum.
 */
uint8_t modem_checksum8(const uint8_t *buf, size_t len);

/**
 * @brief Calculate 16-bit CRC using Zephyr crc16_ccitt service.
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 16-bit CRC value.
 */
static inline uint16_t modem_crc16(const uint8_t *buf, size_t len)
{
    return crc16_ccitt(0x0000, buf, len);
}

/**
 * @brief Update 16-bit CRC using Zephyr crc16_ccitt service.
 * @param crc Previous CRC value.
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return Updated 16-bit CRC value.
 */
static inline uint16_t modem_crc16_update(uint16_t crc, const uint8_t *buf, size_t len)
{
    return crc16_ccitt(crc, buf, len);
}

/**
 * @brief Calculate 32-bit CRC using Zephyr crc32_ieee service.
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return 32-bit CRC value.
 */
static inline uint32_t modem_crc32(const uint8_t *buf, size_t len)
{
    return crc32_ieee(buf, len);
}

/**
 * @brief Update 32-bit CRC using Zephyr crc32_ieee_update service.
 * @param crc Previous raw CRC value.
 * @param buf Pointer to data buffer.
 * @param len Length of buffer in bytes.
 * @return Updated raw CRC value.
 */
static inline uint32_t modem_crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    return crc32_ieee_update(crc, buf, len);
}

/**
 * @brief Finalize 32-bit CRC update.
 * @param crc Raw accumulated CRC value.
 * @return Final 32-bit CRC value.
 */
static inline uint32_t modem_crc32_finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}

#ifdef __cplusplus
}
#endif

#endif /* MODEM_CRC_H_ */
