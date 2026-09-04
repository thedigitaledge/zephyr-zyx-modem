/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Firmware Integrity & Cryptographic Signature Verification for Zephyr OS.
 */

#ifndef MODEM_SIGNATURE_VERIFY_H_
#define MODEM_SIGNATURE_VERIFY_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Signature Verification Context
 */
typedef struct {
    uint32_t current_crc32;
    size_t total_bytes;
    bool verified;
} modem_sig_verify_ctx_t;

/**
 * @brief Initialize signature verification context.
 * @param ctx Verification context pointer.
 * @return 0 on success, negative error code on failure.
 */
int signature_verify_init(modem_sig_verify_ctx_t *ctx);

/**
 * @brief Update verification digest with incoming data chunk.
 * @param ctx Verification context pointer.
 * @param chunk Incoming payload data buffer.
 * @param len Length of payload chunk in bytes.
 * @return 0 on success, negative error code on failure.
 */
int signature_verify_update(modem_sig_verify_ctx_t *ctx, const uint8_t *chunk, size_t len);

/**
 * @brief Finalize and authenticate calculated digest against expected signature.
 * @param ctx Verification context pointer.
 * @param signature Signature buffer (Ed25519 or ECDSA signature bytes).
 * @param sig_len Length of signature buffer.
 * @param public_key Public key buffer.
 * @param key_len Length of public key.
 * @return 0 on successful verification, -1 on authentication failure.
 */
int signature_verify_final(modem_sig_verify_ctx_t *ctx,
                           const uint8_t *signature, size_t sig_len,
                           const uint8_t *public_key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_SIGNATURE_VERIFY_H_ */
