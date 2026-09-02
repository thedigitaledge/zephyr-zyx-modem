/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for sector-by-sector flash erase and wear-leveling helper.
 */

#ifndef MODEM_FLASH_HELPER_H_
#define MODEM_FLASH_HELPER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flash Helper Area Context
 */
typedef struct {
    const char *partition_label;
    size_t current_offset;
    size_t erased_offset;
    size_t sector_size;
} modem_flash_ctx_t;

/**
 * @brief Initialize flash helper area context for target partition.
 * @param ctx Pointer to flash context.
 * @param partition_label Partition name (e.g. "slot1").
 * @return 0 on success, negative error code on failure.
 */
int modem_flash_init(modem_flash_ctx_t *ctx, const char *partition_label);

/**
 * @brief Write data chunk to flash partition, erasing sectors as needed.
 * @param ctx Pointer to flash context.
 * @param buf Data buffer to write.
 * @param len Size of data buffer in bytes.
 * @return 0 on success, negative error code on failure.
 */
int modem_flash_write(modem_flash_ctx_t *ctx, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_FLASH_HELPER_H_ */
