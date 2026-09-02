/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for MCUBoot image magic header and slot boundary validation helper.
 */

#ifndef MODEM_MCUBOOT_VALIDATE_H_
#define MODEM_MCUBOOT_VALIDATE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCUBOOT_IMAGE_MAGIC 0x96F3B83DU

/**
 * @brief MCUBoot Image Header Structure
 */
typedef struct {
    uint32_t magic;      /**< Magic number (0x96F3B83D) */
    uint32_t load_addr;  /**< Load address */
    uint16_t hdr_size;   /**< Header size in bytes */
    uint16_t pad;
    uint32_t img_size;   /**< Image payload size in bytes */
    uint32_t flags;      /**< Image flags */
} mcuboot_image_header_t;

/**
 * @brief Validate MCUBoot image header magic and slot boundary limits.
 * @param buf Pointer to start of binary buffer.
 * @param len Buffer size in bytes.
 * @param max_slot_size Maximum target slot boundary size in bytes.
 * @return 0 if valid MCUBoot image header, negative error code otherwise.
 */
int mcuboot_validate_header(const uint8_t *buf, size_t len, size_t max_slot_size);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_MCUBOOT_VALIDATE_H_ */
