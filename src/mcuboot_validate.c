/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of MCUBoot image magic header and slot boundary validation helper.
 */

#include "mcuboot_validate.h"
#include <string.h>

int mcuboot_validate_header(const uint8_t *buf, size_t len, size_t max_slot_size)
{
    if (!buf || len < sizeof(mcuboot_image_header_t)) {
        return -1;
    }

    const mcuboot_image_header_t *hdr = (const mcuboot_image_header_t *)buf;
    if (hdr->magic != MCUBOOT_IMAGE_MAGIC) {
        return -2; /* Invalid magic header */
    }

    if (max_slot_size > 0 && hdr->img_size > max_slot_size) {
        return -3; /* Oversized image for target slot boundary */
    }

    return 0;
}
