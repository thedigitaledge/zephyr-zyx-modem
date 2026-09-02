/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Header file for Zephyr Console / Shell Serial Modem Integration.
 * Defines high-level API functions for receiving and transmitting files
 * over Zephyr console UART interface.
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
 * Registers modem shell commands (modem rx, ry, rz, sx, sy, sz, and top-level aliases)
 * with the Zephyr shell subsystem.
 *
 * @return 0 on success, negative error code on failure.
 */
int zephyr_console_modem_init(void);

/**
 * @brief Receive a file over console using XMODEM.
 * @param output_filename Target destination path on Zephyr file system.
 * @return 0 on success, negative on error.
 */
int console_modem_rx_xmodem(const char *output_filename);

/**
 * @brief Receive a file over console using YMODEM.
 * @param output_filename Target destination path on Zephyr file system (or NULL to use Block 0 name).
 * @return 0 on success, negative on error.
 */
int console_modem_rx_ymodem(const char *output_filename);

/**
 * @brief Receive a file over console using ZMODEM.
 * @param output_filename Target destination path on Zephyr file system (or NULL to use ZFILE header name).
 * @return 0 on success, negative on error.
 */
int console_modem_rx_zmodem(const char *output_filename);

/**
 * @brief Transmit a file over console using XMODEM.
 * @param input_filename Path to source file on Zephyr file system.
 * @return 0 on success, negative on error.
 */
int console_modem_tx_xmodem(const char *input_filename);

/**
 * @brief Transmit a file over console using YMODEM.
 * @param input_filename Path to source file on Zephyr file system.
 * @return 0 on success, negative on error.
 */
int console_modem_tx_ymodem(const char *input_filename);

/**
 * @brief Transmit a file over console using ZMODEM.
 * @param input_filename Path to source file on Zephyr file system.
 * @return 0 on success, negative on error.
 */
int console_modem_tx_zmodem(const char *input_filename);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZEPHYR_CONSOLE_MODEM_H_ */
