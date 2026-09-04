/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Storage Wear-Aware Log Chunking & Rotation for Zephyr OS.
 */

#include <modem/log_rotation.h>
#include <stdio.h>
#include <string.h>

int log_rotation_init(log_rotation_ctx_t *ctx, const log_rotation_config_t *config)
{
    if (!ctx || !config) {
        return -1;
    }
    ctx->config = *config;
    if (ctx->config.max_chunk_size == 0) {
        ctx->config.max_chunk_size = 65536; /* Default 64KB per chunk */
    }
    if (ctx->config.max_total_chunks == 0) {
        ctx->config.max_total_chunks = 5;
    }
    if (ctx->config.base_filename[0] == '\0') {
        strncpy(ctx->config.base_filename, "system_log", sizeof(ctx->config.base_filename) - 1);
    }
    ctx->current_chunk_index = 0;
    ctx->current_chunk_bytes = 0;
    return 0;
}

int log_rotation_write(log_rotation_ctx_t *ctx, const uint8_t *data, size_t len,
                       char *active_filename, size_t filename_capacity)
{
    if (!ctx || !data) {
        return -1;
    }

    if (ctx->current_chunk_bytes + len > ctx->config.max_chunk_size) {
        ctx->current_chunk_index = (ctx->current_chunk_index + 1) % ctx->config.max_total_chunks;
        ctx->current_chunk_bytes = 0;
    }

    ctx->current_chunk_bytes += len;

    if (active_filename && filename_capacity > 0) {
        snprintf(active_filename, filename_capacity, "%s_%zu.log",
                 ctx->config.base_filename, ctx->current_chunk_index);
    }

    return 0;
}
