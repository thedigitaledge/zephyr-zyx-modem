/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * USB CDC-ACM Channel Adapter Module (Kconfig-conditional).
 */

#include "zephyr_console_modem.h"

#if defined(CONFIG_MODEM_USB_CDC_ACM)
int console_modem_setup_usb_cdc_acm(void)
{
    return 0;
}
#endif
