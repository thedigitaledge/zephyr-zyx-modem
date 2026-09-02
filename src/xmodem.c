/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of XMODEM protocol receiver and transmitter.
 * Supports standard 128-byte block checksum, XMODEM-CRC, and XMODEM-1K variants.
 */

#include "modem/xmodem.h"
#include "crc.h"
#include <string.h>

#define DEFAULT_BYTE_TIMEOUT_MS   1000
#define DEFAULT_PACKET_TIMEOUT_MS 3000
#define DEFAULT_MAX_RETRIES       10

void xmodem_config_init(xmodem_config_t *config)
{
    if (config) {
        config->mode = XMODEM_MODE_1K;
        config->byte_timeout_ms = DEFAULT_BYTE_TIMEOUT_MS;
        config->packet_timeout_ms = DEFAULT_PACKET_TIMEOUT_MS;
        config->max_retries = DEFAULT_MAX_RETRIES;
    }
}

static xmodem_status_t send_byte(const xmodem_callbacks_t *cbs, uint8_t byte)
{
    if (!cbs || !cbs->write_bytes) {
        return XMODEM_ERROR_IO;
    }
    return (cbs->write_bytes(&byte, 1, cbs->user_data) == 0) ? XMODEM_OK : XMODEM_ERROR_IO;
}

static xmodem_status_t purge_rx(const xmodem_callbacks_t *cbs)
{
    uint8_t dummy;
    /* Purge unread bytes from serial transport pipeline until timeout */
    while (cbs->read_byte(&dummy, 100, cbs->user_data) == 0) {
        /* Purge channel */
    }
    return XMODEM_OK;
}

xmodem_status_t xmodem_receive(const xmodem_callbacks_t *callbacks,
                               const xmodem_config_t *config,
                               size_t *total_received)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes || !callbacks->data_cb) {
        return XMODEM_ERROR_IO;
    }

    xmodem_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        xmodem_config_init(&cfg);
    }

    uint8_t expected_block = 1;
    size_t total_bytes = 0;
    uint8_t retries = 0;
    bool crc_mode = (cfg.mode != XMODEM_MODE_STANDARD);

    /* Step 1: Send initial handshake character ('C' for CRC or NAK for checksum) */
    uint8_t req = crc_mode ? XMODEM_C : XMODEM_NAK;
    send_byte(callbacks, req);

    uint8_t payload[XMODEM_BLOCK_SIZE_1024];

    /* Step 2: Main packet reception loop */
    while (1) {
        uint8_t header;
        int res = callbacks->read_byte(&header, cfg.packet_timeout_ms, callbacks->user_data);

        /* Step 2a: Timeout handling and CRC fallback */
        if (res != 0) {
            retries++;
            if (retries > cfg.max_retries) {
                send_byte(callbacks, XMODEM_CAN);
                send_byte(callbacks, XMODEM_CAN);
                return XMODEM_ERROR_TIMEOUT;
            }
            if (total_bytes == 0 && retries > (cfg.max_retries / 2) && crc_mode && cfg.mode != XMODEM_MODE_CRC) {
                /* Fallback to NAK / checksum mode if CRC handshake fails repeatedly */
                crc_mode = false;
                req = XMODEM_NAK;
            }
            send_byte(callbacks, req);
            continue;
        }

        /* Step 2b: Handle End of Transmission (EOT) */
        if (header == XMODEM_EOT) {
            send_byte(callbacks, XMODEM_ACK);
            if (total_received) {
                *total_received = total_bytes;
            }
            return XMODEM_OK;
        }

        /* Step 2c: Handle Cancel signal (CAN) */
        if (header == XMODEM_CAN) {
            uint8_t second_can;
            if (callbacks->read_byte(&second_can, cfg.byte_timeout_ms, callbacks->user_data) == 0 &&
                second_can == XMODEM_CAN) {
                return XMODEM_ERROR_CANCEL;
            }
            return XMODEM_ERROR_CANCEL;
        }

        /* Step 2d: Validate block header size */
        size_t block_len;
        if (header == XMODEM_SOH) {
            block_len = XMODEM_BLOCK_SIZE_128;
        } else if (header == XMODEM_STX) {
            block_len = XMODEM_BLOCK_SIZE_1024;
        } else {
            purge_rx(callbacks);
            retries++;
            if (retries > cfg.max_retries) {
                send_byte(callbacks, XMODEM_CAN);
                return XMODEM_ERROR;
            }
            send_byte(callbacks, XMODEM_NAK);
            continue;
        }

        /* Step 2e: Read and verify block sequence number */
        uint8_t blk_num, blk_num_inv;
        if (callbacks->read_byte(&blk_num, cfg.byte_timeout_ms, callbacks->user_data) != 0 ||
            callbacks->read_byte(&blk_num_inv, cfg.byte_timeout_ms, callbacks->user_data) != 0) {
            purge_rx(callbacks);
            retries++;
            send_byte(callbacks, XMODEM_NAK);
            continue;
        }

        if ((uint8_t)(blk_num + blk_num_inv) != 0xFF) {
            purge_rx(callbacks);
            retries++;
            send_byte(callbacks, XMODEM_NAK);
            continue;
        }

        /* Step 2f: Read block payload bytes */
        bool io_error = false;
        for (size_t i = 0; i < block_len; i++) {
            if (callbacks->read_byte(&payload[i], cfg.byte_timeout_ms, callbacks->user_data) != 0) {
                io_error = true;
                break;
            }
        }

        if (io_error) {
            purge_rx(callbacks);
            retries++;
            send_byte(callbacks, XMODEM_NAK);
            continue;
        }

        /* Step 2g: Check CRC or checksum */
        if (crc_mode) {
            uint8_t crc_hi, crc_lo;
            if (callbacks->read_byte(&crc_hi, cfg.byte_timeout_ms, callbacks->user_data) != 0 ||
                callbacks->read_byte(&crc_lo, cfg.byte_timeout_ms, callbacks->user_data) != 0) {
                purge_rx(callbacks);
                retries++;
                send_byte(callbacks, XMODEM_NAK);
                continue;
            }
            uint16_t expected_crc = ((uint16_t)crc_hi << 8) | crc_lo;
            uint16_t actual_crc = modem_crc16(payload, block_len);

            if (expected_crc != actual_crc) {
                purge_rx(callbacks);
                retries++;
                send_byte(callbacks, XMODEM_NAK);
                continue;
            }
        } else {
            uint8_t cksum;
            if (callbacks->read_byte(&cksum, cfg.byte_timeout_ms, callbacks->user_data) != 0) {
                purge_rx(callbacks);
                retries++;
                send_byte(callbacks, XMODEM_NAK);
                continue;
            }
            uint8_t actual_cksum = modem_checksum8(payload, block_len);
            if (cksum != actual_cksum) {
                purge_rx(callbacks);
                retries++;
                send_byte(callbacks, XMODEM_NAK);
                continue;
            }
        }

        /* Step 2h: Process validated payload or duplicate block */
        if (blk_num == expected_block) {
            if (callbacks->data_cb(expected_block, payload, block_len, callbacks->user_data) != 0) {
                send_byte(callbacks, XMODEM_CAN);
                send_byte(callbacks, XMODEM_CAN);
                return XMODEM_ERROR_IO;
            }
            total_bytes += block_len;
            expected_block++;
            retries = 0;
            req = XMODEM_ACK;
            send_byte(callbacks, XMODEM_ACK);
        } else if (blk_num == (uint8_t)(expected_block - 1)) {
            /* Duplicate block received; acknowledge to keep sync */
            send_byte(callbacks, XMODEM_ACK);
        } else {
            send_byte(callbacks, XMODEM_CAN);
            send_byte(callbacks, XMODEM_CAN);
            return XMODEM_ERROR_SEQUENCE;
        }
    }
}

xmodem_status_t xmodem_transmit(const xmodem_callbacks_t *callbacks,
                                size_t total_len,
                                const xmodem_config_t *config)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes || !callbacks->data_cb) {
        return XMODEM_ERROR_IO;
    }

    xmodem_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        xmodem_config_init(&cfg);
    }

    /* Step 1: Wait for receiver start signal ('C' or NAK) */
    uint8_t start_byte;
    uint8_t retries = 0;
    bool crc_mode = true;

    while (1) {
        if (callbacks->read_byte(&start_byte, cfg.packet_timeout_ms, callbacks->user_data) == 0) {
            if (start_byte == XMODEM_C) {
                crc_mode = true;
                break;
            } else if (start_byte == XMODEM_NAK) {
                crc_mode = false;
                break;
            } else if (start_byte == XMODEM_CAN) {
                return XMODEM_ERROR_CANCEL;
            }
        }
        retries++;
        if (retries > cfg.max_retries) {
            return XMODEM_ERROR_TIMEOUT;
        }
    }

    uint8_t block_num = 1;
    size_t bytes_sent = 0;
    uint8_t payload[XMODEM_BLOCK_SIZE_1024];

    /* Step 2: Transmit file data blocks */
    while (total_len == 0 || bytes_sent < total_len) {
        size_t block_size = (cfg.mode == XMODEM_MODE_1K) ? XMODEM_BLOCK_SIZE_1024 : XMODEM_BLOCK_SIZE_128;
        if (total_len > 0 && (total_len - bytes_sent) < block_size && cfg.mode != XMODEM_MODE_1K) {
            block_size = XMODEM_BLOCK_SIZE_128;
        }

        memset(payload, 0x1A, block_size); /* CPM EOF padding byte (0x1A) */

        int cb_res = callbacks->data_cb(block_num, payload, block_size, callbacks->user_data);
        if (cb_res < 0) {
            send_byte(callbacks, XMODEM_CAN);
            return XMODEM_ERROR_IO;
        }

        if (cb_res == 0 && total_len == 0 && bytes_sent > 0) {
            break;
        }

        retries = 0;
        bool acked = false;

        while (retries < cfg.max_retries && !acked) {
            uint8_t pkt_header = (block_size == XMODEM_BLOCK_SIZE_1024) ? XMODEM_STX : XMODEM_SOH;
            send_byte(callbacks, pkt_header);
            send_byte(callbacks, block_num);
            send_byte(callbacks, (uint8_t)~block_num);
            callbacks->write_bytes(payload, block_size, callbacks->user_data);

            if (crc_mode) {
                uint16_t crc = modem_crc16(payload, block_size);
                uint8_t crc_buf[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
                callbacks->write_bytes(crc_buf, 2, callbacks->user_data);
            } else {
                uint8_t cksum = modem_checksum8(payload, block_size);
                send_byte(callbacks, cksum);
            }

            uint8_t rx_resp;
            if (callbacks->read_byte(&rx_resp, cfg.packet_timeout_ms, callbacks->user_data) == 0) {
                if (rx_resp == XMODEM_ACK) {
                    acked = true;
                } else if (rx_resp == XMODEM_CAN) {
                    return XMODEM_ERROR_CANCEL;
                }
            }
            if (!acked) {
                retries++;
            }
        }

        if (!acked) {
            send_byte(callbacks, XMODEM_CAN);
            return XMODEM_ERROR_TIMEOUT;
        }

        bytes_sent += block_size;
        block_num++;
    }

    /* Step 3: Send End of Transmission (EOT) */
    retries = 0;
    while (retries < cfg.max_retries) {
        send_byte(callbacks, XMODEM_EOT);
        uint8_t resp;
        if (callbacks->read_byte(&resp, cfg.packet_timeout_ms, callbacks->user_data) == 0 && resp == XMODEM_ACK) {
            return XMODEM_OK;
        }
        retries++;
    }

    return XMODEM_ERROR_TIMEOUT;
}
