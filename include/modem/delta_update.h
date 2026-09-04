/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Binary Delta Update Engine for Zephyr OS.
 */

#ifndef MODEM_DELTA_UPDATE_H_
#define MODEM_DELTA_UPDATE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delta Patch Context
 */
typedef struct {
    size_t base_image_size;
    size_t patch_offset;
    size_t target_written;
} modem_delta_ctx_t;

/**
 * @brief Initialize binary delta patch context.
 * @param ctx Delta patch context pointer.
 * @param base_size Size of base image in bytes.
 * @return 0 on success, negative error code on failure.
 */
int delta_update_init(modem_delta_ctx_t *ctx, size_t base_size);

/**
 * @brief Apply binary diff chunk onto base image stream.
 * @param ctx Delta patch context pointer.
 * @param diff_chunk Incoming binary patch data chunk.
 * @param chunk_len Length of patch data chunk.
 * @param base_buf Reference base image memory buffer.
 * @param out_buf Target buffer to write patched binary output.
 * @param out_capacity Target buffer capacity.
 * @param out_len Output pointer for generated output bytes.
 * @return 0 on success, negative error code on failure.
 */
int delta_update_apply_chunk(modem_delta_ctx_t *ctx,
                             const uint8_t *diff_chunk, size_t chunk_len,
                             const uint8_t *base_buf, uint8_t *out_buf,
                             size_t out_capacity, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_DELTA_UPDATE_H_ */
