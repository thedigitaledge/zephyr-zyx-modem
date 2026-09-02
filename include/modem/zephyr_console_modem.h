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
 * Registers modem shell commands (modem rx, modem ry, modem rz, rx, ry, rz)
 * with the Zephyr shell subsystem.
 *
 * @return 0 on success, negative error code on failure.
 */
int zephyr_console_modem_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_ZEPHYR_CONSOLE_MODEM_H_ */
