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
    bool flow_control;             /**< RTS/CTS hardware and XON/XOFF software flow control */
    char flash_partition[32];      /**< Target raw flash area partition name (e.g. slot1) */
    bool mcuboot_update;           /**< MCUBoot dual-bank image upgrade manager */
    bool nvs_checkpoints;          /**< NVS / Settings auto-resume checkpoints */
    bool crypto_stream;            /**< In-flight payload streaming encryption */
    bool uart_dma;                 /**< High-speed zero-copy async UART DMA ring buffer adapter */
    bool usb_cdc_acm;              /**< USB CDC-ACM virtual serial port adapter */
    bool mcuboot_validate;         /**< MCUBoot image magic header and slot boundary validation */
    bool signature_verify;         /**< Firmware signature verification */
    bool encrypted_envelope;       /**< Encrypted stream payload envelope */
    bool session_dispatcher;       /**< Multi-session transport dispatcher */
    bool log_rotation;             /**< Wear-aware log rotation */
} console_modem_settings_t;

/**
 * @brief Initialize USB CDC-ACM virtual serial port device channel.
 * @return 0 on success, negative error code on failure.
 */
int console_modem_setup_usb_cdc_acm(void);

/**
 * @brief Stream MCUBoot image update to secondary partition slot.
 * @param output_filename Image file or target slot name.
 * @param protocol Protocol selector (0=ZMODEM, 1=YMODEM, 2=XMODEM).
 * @return 0 on success, negative error code on failure.
 */
int console_modem_mcuboot_update(const char *output_filename, int protocol);

/**
 * @brief Transfer Statistics Counters
 */
typedef struct {
    uint32_t total_transfers;
    uint32_t successful_transfers;
    uint32_t failed_transfers;
    uint32_t crc_errors;
    uint32_t retries;
    size_t total_bytes_rx;
    size_t total_bytes_tx;
} modem_stats_t;

/**
 * @brief Get cumulative modem transfer statistics counters.
 * @param stats Output pointer for stats structure.
 */
void console_modem_stats_get(modem_stats_t *stats);

/**
 * @brief Reset cumulative modem transfer statistics counters.
 */
void console_modem_stats_reset(void);

/**
 * @brief Channel device binding context for multi-UART / multi-transport instances.
 */
typedef struct {
    const struct device *uart_dev;
    int (*read_byte)(uint8_t *byte, uint32_t timeout_ms, void *user_data);
    int (*write_bytes)(const uint8_t *buf, size_t len, void *user_data);
    void *user_data;
} console_modem_channel_t;

/**
 * @brief Bind serial transfer routines to custom device channel instance.
 * @param channel Pointer to channel binding context structure.
 * @return 0 on success, negative on error.
 */
int console_modem_bind_device(console_modem_channel_t *channel);

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
