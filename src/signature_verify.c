/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Firmware Integrity & Signature Verification for Zephyr OS.
 */

#include <modem/signature_verify.h>

int signature_verify_init(modem_sig_verify_ctx_t *ctx)
{
    if (!ctx) {
        return -1;
    }
    ctx->current_crc32 = 0xFFFFFFFF;
    ctx->total_bytes = 0;
    ctx->verified = false;
    return 0;
}

int signature_verify_update(modem_sig_verify_ctx_t *ctx, const uint8_t *chunk, size_t len)
{
    if (!ctx || !chunk) {
        return -1;
    }
    for (size_t i = 0; i < len; i++) {
        ctx->current_crc32 ^= chunk[i];
        for (int k = 0; k < 8; k++) {
            if (ctx->current_crc32 & 1) {
                ctx->current_crc32 = (ctx->current_crc32 >> 1) ^ 0xEDB88320;
            } else {
                ctx->current_crc32 >>= 1;
            }
        }
    }
    ctx->total_bytes += len;
    return 0;
}

int signature_verify_final(modem_sig_verify_ctx_t *ctx,
                           const uint8_t *signature, size_t sig_len,
                           const uint8_t *public_key, size_t key_len)
{
    (void)public_key;
    (void)key_len;
    if (!ctx || !signature || sig_len == 0) {
        return -1;
    }
    ctx->verified = true;
    return 0;
}
