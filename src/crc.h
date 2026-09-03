/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file defining CRC calculation wrappers for serial transfer protocols.
 */

#ifndef MODEM_CRC_H_
#define MODEM_CRC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/crc.h>

#if defined(CONFIG_MODEM_HW_CRC)
#include <zephyr/drivers/crc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate simple 8-bit arithmetic checksum.
 *
 * Iterates through the buffer accumulating byte values modulo 256.
 * Used in basic XMODEM 128-byte checksum mode.
 *
 * @param buf Pointer to input data buffer.
 * @param len Length of buffer in bytes.
 * @return 8-bit arithmetic checksum.
 */
uint8_t modem_checksum8(const uint8_t *buf, size_t len);

/**
 * @brief Calculate 16-bit CRC using Zephyr's crc16_ccitt service.
 *
 * Computes 16-bit CCITT CRC (polynomial 0x1021, init 0x0000).
 * Used in XMODEM-CRC, XMODEM-1K, YMODEM, and ZMODEM16.
 *
 * @param buf Pointer to input data buffer.
 * @param len Length of buffer in bytes.
 * @return Computed 16-bit CRC value.
 */
static inline uint16_t modem_crc16(const uint8_t *buf, size_t len)
{
    return crc16_ccitt(0x0000, buf, len);
}

/**
 * @brief Update ongoing 16-bit CRC using Zephyr's crc16_ccitt service.
 *
 * Accumulates CRC value across multiple data chunks.
 *
 * @param crc Previous accumulated CRC value.
 * @param buf Pointer to input data buffer chunk.
 * @param len Length of buffer chunk in bytes.
 * @return Updated 16-bit CRC value.
 */
static inline uint16_t modem_crc16_update(uint16_t crc, const uint8_t *buf, size_t len)
{
    return crc16_ccitt(crc, buf, len);
}

/**
 * @brief Calculate 32-bit CRC using Zephyr's crc32_ieee service.
 *
 * Computes IEEE 802.3 32-bit CRC.
 * Used in ZMODEM32 streaming transfers.
 *
 * @param buf Pointer to input data buffer.
 * @param len Length of buffer in bytes.
 * @return Computed 32-bit CRC value.
 */
static inline uint32_t modem_crc32(const uint8_t *buf, size_t len)
{
    return crc32_ieee(buf, len);
}

/**
 * @brief Update ongoing 32-bit CRC using Zephyr's crc32_ieee_update service.
 *
 * Accumulates raw 32-bit CRC value across streaming frames.
 *
 * @param crc Previous raw accumulated CRC value.
 * @param buf Pointer to input data buffer chunk.
 * @param len Length of buffer chunk in bytes.
 * @return Updated raw 32-bit CRC value.
 */
static inline uint32_t modem_crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    return crc32_ieee_update(crc, buf, len);
}

/**
 * @brief Finalize accumulated 32-bit CRC update.
 *
 * Applies final XOR mask (0xFFFFFFFF).
 *
 * @param crc Raw accumulated CRC value from modem_crc32_update.
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
