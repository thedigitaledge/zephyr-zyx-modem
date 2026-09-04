/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Binary Delta Update Engine for Zephyr OS.
 */

#include <modem/delta_update.h>
#include <string.h>

int delta_update_init(modem_delta_ctx_t *ctx, size_t base_size)
{
    if (!ctx) {
        return -1;
    }
    ctx->base_image_size = base_size;
    ctx->patch_offset = 0;
    ctx->target_written = 0;
    return 0;
}

int delta_update_apply_chunk(modem_delta_ctx_t *ctx,
                             const uint8_t *diff_chunk, size_t chunk_len,
                             const uint8_t *base_buf, uint8_t *out_buf,
                             size_t out_capacity, size_t *out_len)
{
    if (!ctx || !diff_chunk || !out_buf || !out_len) {
        return -1;
    }

    size_t copy_len = (chunk_len < out_capacity) ? chunk_len : out_capacity;

    if (base_buf) {
        for (size_t i = 0; i < copy_len; i++) {
            out_buf[i] = base_buf[ctx->target_written + i] ^ diff_chunk[i];
        }
    } else {
        memcpy(out_buf, diff_chunk, copy_len);
    }

    *out_len = copy_len;
    ctx->target_written += copy_len;
    ctx->patch_offset += chunk_len;
    return 0;
}
