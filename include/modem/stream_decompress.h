/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Stream Decompression Engine (Heatshrink / RLE) for Zephyr OS.
 */

#ifndef MODEM_STREAM_DECOMPRESS_H_
#define MODEM_STREAM_DECOMPRESS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compression Algorithm Types
 */
typedef enum {
    MODEM_COMPRESS_NONE = 0,
    MODEM_COMPRESS_RLE,
    MODEM_COMPRESS_HEATSHRINK,
    MODEM_COMPRESS_LZ4
} modem_compress_type_t;

/**
 * @brief Decompression Context
 */
typedef struct {
    modem_compress_type_t algo;
    size_t processed_bytes;
} modem_decompress_ctx_t;

/**
 * @brief Initialize decompression context.
 * @param ctx Pointer to decompression context.
 * @param algo Selected compression algorithm.
 * @return 0 on success, negative error code on failure.
 */
int stream_decompress_init(modem_decompress_ctx_t *ctx, modem_compress_type_t algo);

/**
 * @brief Process and decompress chunk of compressed input data.
 * @param ctx Pointer to decompression context.
 * @param in_buf Input compressed bytes buffer.
 * @param in_len Input byte length.
 * @param out_buf Output decompressed buffer.
 * @param out_capacity Capacity of output buffer.
 * @param out_produced Output pointer for number of decompressed bytes written.
 * @return 0 on success, negative error code on failure.
 */
int stream_decompress_process(modem_decompress_ctx_t *ctx,
                              const uint8_t *in_buf, size_t in_len,
                              uint8_t *out_buf, size_t out_capacity,
                              size_t *out_produced);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_STREAM_DECOMPRESS_H_ */
