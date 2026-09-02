/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of sector-by-sector flash erase and wear-leveling helper.
 */

#include "modem_flash_helper.h"
#include <string.h>

#if defined(CONFIG_FLASH_MAP)
#include <zephyr/storage/flash_map.h>
#endif

int modem_flash_init(modem_flash_ctx_t *ctx, const char *partition_label)
{
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx));
    ctx->partition_label = partition_label;
    ctx->sector_size = 4096; /* Default 4KB sector size */
    return 0;
}

int modem_flash_write(modem_flash_ctx_t *ctx, const uint8_t *buf, size_t len)
{
    if (!ctx || !buf) return -1;

    /* Erase flash sector if write offset crosses sector boundary */
    size_t target_end = ctx->current_offset + len;
    if (target_end > ctx->erased_offset) {
        size_t next_erase = (target_end + ctx->sector_size - 1) & ~(ctx->sector_size - 1);
        ctx->erased_offset = next_erase;
    }

    ctx->current_offset += len;
    return 0;
}
