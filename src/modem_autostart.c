/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * ZMODEM Auto-Start Detection Handler Module (Kconfig-conditional).
 */

#include "zephyr_console_modem.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static uint8_t g_autostart_buf[8] = {0};
static size_t g_autostart_idx = 0;

bool console_modem_check_autostart(uint8_t byte)
{
#if defined(CONFIG_MODEM_AUTO_START)
    g_autostart_buf[g_autostart_idx % 8] = byte;
    g_autostart_idx++;
    if (g_autostart_idx >= 3) {
        size_t idx = (g_autostart_idx - 3) % 8;
        if (g_autostart_buf[idx] == 'r' &&
            g_autostart_buf[(idx + 1) % 8] == 'z' &&
            g_autostart_buf[(idx + 2) % 8] == '\r') {
            return true;
        }
    }
#else
    (void)byte;
#endif
    return false;
}
