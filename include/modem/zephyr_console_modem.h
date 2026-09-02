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

/** Receive functions */
int console_modem_rx_xmodem(const char *output_filename);
int console_modem_rx_ymodem(const char *output_filename);
int console_modem_rx_zmodem(const char *output_filename);

/** Transmit functions */
int console_modem_tx_xmodem(const char *input_filename);
int console_modem_tx_ymodem(const char *input_filename);
int console_modem_tx_zmodem(const char *input_filename);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZEPHYR_CONSOLE_MODEM_H_ */
