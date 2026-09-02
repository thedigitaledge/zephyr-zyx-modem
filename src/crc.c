/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of 8-bit arithmetic checksum utility.
 */

#include "crc.h"

uint8_t modem_checksum8(const uint8_t *buf, size_t len)
{
    uint8_t sum = 0;
    if (!buf) {
        return 0;
    }
    /* Accumulate raw sum modulo 256 for 8-bit arithmetic checksum */
    for (size_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}
