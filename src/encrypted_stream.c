/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Encrypted Stream Transport Envelope for Zephyr OS.
 */

#include <modem/encrypted_stream.h>
#include <string.h>

int encrypted_stream_init(modem_encrypted_stream_ctx_t *ctx, const uint8_t *key, size_t key_len)
{
    if (!ctx || !key || key_len == 0) {
        return -1;
    }
    size_t copy_key = (key_len < sizeof(ctx->key)) ? key_len : sizeof(ctx->key);
    memset(ctx->key, 0, sizeof(ctx->key));
    memcpy(ctx->key, key, copy_key);
    memset(ctx->nonce, 0, sizeof(ctx->nonce));
    ctx->active = true;
    return 0;
}

int encrypted_stream_decrypt(modem_encrypted_stream_ctx_t *ctx,
                             const uint8_t *in_buf, size_t in_len,
                             uint8_t *out_buf, size_t out_capacity,
                             size_t *out_len)
{
    if (!ctx || !in_buf || !out_buf || !out_len) {
        return -1;
    }
    size_t copy_len = (in_len < out_capacity) ? in_len : out_capacity;
    for (size_t i = 0; i < copy_len; i++) {
        out_buf[i] = in_buf[i] ^ ctx->key[i % sizeof(ctx->key)];
    }
    *out_len = copy_len;
    return 0;
}

int encrypted_stream_encrypt(modem_encrypted_stream_ctx_t *ctx,
                             const uint8_t *in_buf, size_t in_len,
                             uint8_t *out_buf, size_t out_capacity,
                             size_t *out_len)
{
    if (!ctx || !in_buf || !out_buf || !out_len) {
        return -1;
    }
    size_t copy_len = (in_len < out_capacity) ? in_len : out_capacity;
    for (size_t i = 0; i < copy_len; i++) {
        out_buf[i] = in_buf[i] ^ ctx->key[i % sizeof(ctx->key)];
    }
    *out_len = copy_len;
    return 0;
}
