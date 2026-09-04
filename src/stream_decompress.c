/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Stream Decompression Engine for Zephyr OS.
 */

#include <modem/stream_decompress.h>
#include <string.h>

int stream_decompress_init(modem_decompress_ctx_t *ctx, modem_compress_type_t algo)
{
    if (!ctx) {
        return -1;
    }
    ctx->algo = algo;
    ctx->processed_bytes = 0;
    return 0;
}

int stream_decompress_process(modem_decompress_ctx_t *ctx,
                              const uint8_t *in_buf, size_t in_len,
                              uint8_t *out_buf, size_t out_capacity,
                              size_t *out_produced)
{
    if (!ctx || !in_buf || !out_buf || !out_produced) {
        return -1;
    }

    if (ctx->algo == MODEM_COMPRESS_NONE) {
        size_t copy_len = (in_len < out_capacity) ? in_len : out_capacity;
        memcpy(out_buf, in_buf, copy_len);
        *out_produced = copy_len;
        ctx->processed_bytes += copy_len;
        return 0;
    }

    if (ctx->algo == MODEM_COMPRESS_RLE) {
        size_t out_idx = 0;
        size_t in_idx = 0;

        while (in_idx < in_len && out_idx < out_capacity) {
            if (in_idx + 1 < in_len && in_buf[in_idx] == 0x90 && in_buf[in_idx + 1] != 0x00) {
                uint8_t count = in_buf[in_idx + 1];
                uint8_t val = (out_idx > 0) ? out_buf[out_idx - 1] : 0;
                for (uint8_t k = 0; k < count && out_idx < out_capacity; k++) {
                    out_buf[out_idx++] = val;
                }
                in_idx += 2;
            } else {
                out_buf[out_idx++] = in_buf[in_idx++];
            }
        }
        *out_produced = out_idx;
        ctx->processed_bytes += out_idx;
        return 0;
    }

    /* Default passthrough fallback */
    size_t copy_len = (in_len < out_capacity) ? in_len : out_capacity;
    memcpy(out_buf, in_buf, copy_len);
    *out_produced = copy_len;
    ctx->processed_bytes += copy_len;
    return 0;
}
