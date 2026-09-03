/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Modem Transfer Statistics Module (Kconfig-conditional).
 */

#include "zephyr_console_modem.h"
#include <string.h>

static modem_stats_t g_modem_stats = {0};

void console_modem_stats_get(modem_stats_t *stats)
{
    if (stats) {
        *stats = g_modem_stats;
    }
}

void console_modem_stats_reset(void)
{
    memset(&g_modem_stats, 0, sizeof(g_modem_stats));
}
