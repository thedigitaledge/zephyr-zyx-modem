/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Storage Wear-Aware Log Chunking & Rotation for Zephyr OS.
 */

#ifndef MODEM_LOG_ROTATION_H_
#define MODEM_LOG_ROTATION_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log Rotation Configuration
 */
typedef struct {
    size_t max_chunk_size;    /**< Maximum size in bytes per log chunk file */
    size_t max_total_chunks;  /**< Maximum number of rotated chunk files to keep */
    char base_filename[128];  /**< Base log filename prefix (e.g. "log") */
} log_rotation_config_t;

/**
 * @brief Log Rotation Context
 */
typedef struct {
    size_t current_chunk_index;
    size_t current_chunk_bytes;
    log_rotation_config_t config;
} log_rotation_ctx_t;

/**
 * @brief Initialize log rotation context.
 * @param ctx Pointer to log rotation context.
 * @param config Configuration parameters.
 * @return 0 on success, negative error code on failure.
 */
int log_rotation_init(log_rotation_ctx_t *ctx, const log_rotation_config_t *config);

/**
 * @brief Write log data chunk with automatic file rotation and flash wear balancing.
 * @param ctx Pointer to log rotation context.
 * @param data Input log bytes buffer.
 * @param len Length of log bytes.
 * @param active_filename Output buffer receiving current chunk filename.
 * @param filename_capacity Capacity of output filename buffer.
 * @return 0 on success, negative error code on failure.
 */
int log_rotation_write(log_rotation_ctx_t *ctx, const uint8_t *data, size_t len,
                       char *active_filename, size_t filename_capacity);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_LOG_ROTATION_H_ */
