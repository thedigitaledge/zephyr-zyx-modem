/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Encrypted Stream Transport Envelope (AES-GCM / ChaCha20) for Zephyr OS.
 */

#ifndef MODEM_ENCRYPTED_STREAM_H_
#define MODEM_ENCRYPTED_STREAM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Encrypted Stream Context
 */
typedef struct {
    uint8_t key[32];
    uint8_t nonce[12];
    bool active;
} modem_encrypted_stream_ctx_t;

/**
 * @brief Initialize encrypted stream transport context.
 * @param ctx Encrypted stream context pointer.
 * @param key Pre-shared key buffer (32 bytes).
 * @param key_len Length of key in bytes.
 * @return 0 on success, negative error code on failure.
 */
int encrypted_stream_init(modem_encrypted_stream_ctx_t *ctx, const uint8_t *key, size_t key_len);

/**
 * @brief Decrypt incoming encrypted payload frame.
 * @param ctx Encrypted stream context pointer.
 * @param in_buf Ciphertext input buffer.
 * @param in_len Ciphertext byte length.
 * @param out_buf Plaintext output buffer.
 * @param out_capacity Plaintext output buffer capacity.
 * @param out_len Output pointer for decrypted byte length.
 * @return 0 on success, negative error code on failure.
 */
int encrypted_stream_decrypt(modem_encrypted_stream_ctx_t *ctx,
                             const uint8_t *in_buf, size_t in_len,
                             uint8_t *out_buf, size_t out_capacity,
                             size_t *out_len);

/**
 * @brief Encrypt outgoing plaintext payload frame.
 * @param ctx Encrypted stream context pointer.
 * @param in_buf Plaintext input buffer.
 * @param in_len Plaintext byte length.
 * @param out_buf Ciphertext output buffer.
 * @param out_capacity Ciphertext output buffer capacity.
 * @param out_len Output pointer for encrypted byte length.
 * @return 0 on success, negative error code on failure.
 */
int encrypted_stream_encrypt(modem_encrypted_stream_ctx_t *ctx,
                             const uint8_t *in_buf, size_t in_len,
                             uint8_t *out_buf, size_t out_capacity,
                             size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ENCRYPTED_STREAM_H_ */
