/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Header file for Zephyr Console / Shell Serial Modem Integration.
 * Defines high-level API functions for receiving, transmitting,
 * and configuring file transfer timeouts over Zephyr console.
 */

#ifndef MODEM_ZEPHYR_CONSOLE_MODEM_H_
#define MODEM_ZEPHYR_CONSOLE_MODEM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Zephyr console modem commands.
 *
 * Registers modem shell commands with the Zephyr shell subsystem.
 *
 * @return 0 on success, negative error code on failure.
 */
int zephyr_console_modem_init(void);

/**
 * @brief Runtime Modem Transfer Settings
 */
typedef struct {
    uint32_t packet_timeout_ms; /**< Timeout waiting for packet responses */
    uint32_t byte_timeout_ms;   /**< Timeout waiting for next byte */
    uint8_t max_retries;        /**< Maximum retries per packet */
} console_modem_settings_t;

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

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZEPHYR_CONSOLE_MODEM_H_ */
