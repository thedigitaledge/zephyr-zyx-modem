/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Zephyr Console / Shell Serial Modem Integration.
 * Defines high-level API functions for receiving, transmitting,
 * and configuring file transfer timeouts over Zephyr console.
 */

#ifndef MODEM_ZEPHYR_CONSOLE_MODEM_H_
#define MODEM_ZEPHYR_CONSOLE_MODEM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief File Overwrite Policies
 */
typedef enum {
    MODEM_OVERWRITE_ALWAYS = 0, /**< Overwrite existing file */
    MODEM_OVERWRITE_SKIP   = 1, /**< Skip transfer if file exists */
    MODEM_OVERWRITE_ABORT  = 2  /**< Abort transfer if file exists */
} modem_overwrite_mode_t;

/**
 * @brief Runtime Modem Transfer Settings
 */
typedef struct {
    uint32_t packet_timeout_ms;    /**< Timeout waiting for packet responses */
    uint32_t byte_timeout_ms;      /**< Timeout waiting for next byte */
    uint8_t max_retries;           /**< Maximum retries per packet */
    uint32_t inter_block_delay_ms; /**< Delay inserted between transmitted blocks */
    uint32_t handshake_delay_ms;   /**< Delay between handshake attempts */
    modem_overwrite_mode_t overwrite_mode; /**< File overwrite policy */
    bool enable_resume;            /**< Enable ZMODEM auto-resume */
    char default_target_dir[128];  /**< Default storage directory path */
    uint32_t sync_interval_blocks; /**< Interval in blocks between file syncs */
    bool auto_start;               /**< Enable ZMODEM auto-start detection */
    bool async_storage;            /**< Offload file writes to Zephyr workqueue */
    bool progress_bar;             /**< Render shell progress bar and throughput */
    bool directory_transfers;      /**< Enable directory batch transfers */
    bool ring_buffer;              /**< Enable ring buffer UART transport adapter */
    bool abort_key;                /**< Enable terminal abort key monitoring */
    uint8_t abort_key_char;        /**< Configurable abort key ASCII byte value (default 0x03) */
} console_modem_settings_t;

/**
 * @brief Initialize Zephyr console modem commands.
 *
 * Registers modem shell commands with the Zephyr shell subsystem.
 *
 * @return 0 on success, negative error code on failure.
 */
int zephyr_console_modem_init(void);

/**
 * @brief Get current runtime modem settings.
 * @param settings Output pointer for current settings.
 */
void console_modem_settings_get(console_modem_settings_t *settings);

/**
 * @brief Update runtime modem settings.
 * @param settings Pointer to new settings.
 */
void console_modem_settings_set(const console_modem_settings_t *settings);

/** File Receive Functions (Available when CONFIG_FILE_SYSTEM is enabled) */
int console_modem_rx_xmodem(const char *output_filename);
int console_modem_rx_ymodem(const char *output_filename);
int console_modem_rx_zmodem(const char *output_filename);

/** File Transmit Functions (Available when CONFIG_FILE_SYSTEM is enabled) */
int console_modem_tx_xmodem(const char *input_filename);
int console_modem_tx_ymodem(const char *input_filename);
int console_modem_tx_zmodem(const char *input_filename);

/** Advanced Feature Functions */
int console_modem_tx_directory(const char *dir_path, int protocol);
bool console_modem_check_autostart(uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZEPHYR_CONSOLE_MODEM_H_ */
