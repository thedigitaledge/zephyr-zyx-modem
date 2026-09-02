/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of YMODEM batch file transfer protocol receiver and transmitter.
 * Implements Block 0 file metadata negotiation and multi-file batch transfers.
 */

#include "modem/ymodem.h"
#include "modem/xmodem.h"
#include "crc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PACKET_TIMEOUT_MS 3000
#define BYTE_TIMEOUT_MS   1000
#define MAX_RETRIES       10

static int send_byte_tx(const ymodem_tx_callbacks_t *cbs, uint8_t byte)
{
    if (!cbs || !cbs->write_bytes) return -1;
    return cbs->write_bytes(&byte, 1, cbs->user_data);
}

static int send_byte_rx(const ymodem_rx_callbacks_t *cbs, uint8_t byte)
{
    if (!cbs || !cbs->write_bytes) return -1;
    return cbs->write_bytes(&byte, 1, cbs->user_data);
}

static void purge_rx(const ymodem_rx_callbacks_t *cbs)
{
    uint8_t dummy;
    while (cbs->read_byte(&dummy, 100, cbs->user_data) == 0) {}
}

/**
 * Parse YMODEM Block 0 payload containing filename and file length.
 * Block 0 layout: <filename>\0<file length in ASCII decimal>\0...
 */
static void parse_block0(const uint8_t *payload, size_t len, ymodem_file_info_t *info)
{
    memset(info, 0, sizeof(*info));
    if (payload[0] == 0) {
        /* Empty filename field denotes end of YMODEM batch */
        return;
    }

    size_t name_len = strnlen((const char *)payload, len);
    if (name_len >= sizeof(info->filename)) {
        name_len = sizeof(info->filename) - 1;
    }
    memcpy(info->filename, payload, name_len);
    info->filename[name_len] = '\0';

    size_t pos = name_len + 1;
    if (pos < len && payload[pos] != 0) {
        /* Parse file length string in decimal ASCII format */
        info->size = (size_t)strtoul((const char *)&payload[pos], NULL, 10);
    }
}

ymodem_status_t ymodem_receive(const ymodem_rx_callbacks_t *callbacks)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes) {
        return YMODEM_ERROR_IO;
    }

    while (1) {
        /* Step 1: Send 'C' to request Block 0 (Header block) */
        uint8_t retries = 0;
        uint8_t header = 0;
        uint8_t payload[1024];
        bool header_received = false;

        send_byte_rx(callbacks, XMODEM_C);

        while (retries < MAX_RETRIES && !header_received) {
            int hres = callbacks->read_byte(&header, PACKET_TIMEOUT_MS, callbacks->user_data);
            if (hres == -2) {
                send_byte_rx(callbacks, XMODEM_CAN);
                send_byte_rx(callbacks, XMODEM_CAN);
                return YMODEM_ERROR_CANCEL;
            }
            if (hres != 0) {
                retries++;
                send_byte_rx(callbacks, XMODEM_C);
                continue;
            }

            if (header == XMODEM_CAN) {
                return YMODEM_ERROR_CANCEL;
            }

            if (header != XMODEM_SOH && header != XMODEM_STX) {
                purge_rx(callbacks);
                retries++;
                send_byte_rx(callbacks, XMODEM_C);
                continue;
            }

            size_t block_size = (header == XMODEM_STX) ? 1024 : 128;
            uint8_t blk_num, blk_num_inv;
            if (callbacks->read_byte(&blk_num, BYTE_TIMEOUT_MS, callbacks->user_data) != 0 ||
                callbacks->read_byte(&blk_num_inv, BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                purge_rx(callbacks);
                retries++;
                send_byte_rx(callbacks, XMODEM_C);
                continue;
            }

            bool io_err = false;
            for (size_t i = 0; i < block_size; i++) {
                if (callbacks->read_byte(&payload[i], BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                    io_err = true;
                    break;
                }
            }
            uint8_t crc_hi, crc_lo;
            if (callbacks->read_byte(&crc_hi, BYTE_TIMEOUT_MS, callbacks->user_data) != 0 ||
                callbacks->read_byte(&crc_lo, BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                io_err = true;
            }

            if (io_err) {
                purge_rx(callbacks);
                retries++;
                send_byte_rx(callbacks, XMODEM_C);
                continue;
            }

            uint16_t exp_crc = ((uint16_t)crc_hi << 8) | crc_lo;
            if (exp_crc != modem_crc16(payload, block_size) || blk_num != 0) {
                purge_rx(callbacks);
                retries++;
                send_byte_rx(callbacks, XMODEM_C);
                continue;
            }

            header_received = true;
        }

        if (!header_received) {
            send_byte_rx(callbacks, XMODEM_CAN);
            return YMODEM_ERROR_TIMEOUT;
        }

        ymodem_file_info_t info;
        parse_block0(payload, 128, &info);

        if (info.filename[0] == '\0') {
            /* Empty filename in Block 0 signifies end of YMODEM batch */
            send_byte_rx(callbacks, XMODEM_ACK);
            return YMODEM_OK;
        }

        /* ACK Block 0 and send 'C' to request data transmission */
        send_byte_rx(callbacks, XMODEM_ACK);
        send_byte_rx(callbacks, XMODEM_C);

        if (callbacks->on_file_start) {
            callbacks->on_file_start(&info, callbacks->user_data);
        }

        /* Step 2: Receive file contents using XMODEM 1K state logic */
        uint8_t expected_blk = 1;
        size_t file_offset = 0;
        bool file_done = false;

        while (!file_done) {
            uint8_t pkt_hdr;
            int pres = callbacks->read_byte(&pkt_hdr, PACKET_TIMEOUT_MS, callbacks->user_data);
            if (pres == -2) {
                send_byte_rx(callbacks, XMODEM_CAN);
                send_byte_rx(callbacks, XMODEM_CAN);
                if (callbacks->on_file_end) {
                    callbacks->on_file_end(&info, YMODEM_ERROR_CANCEL, callbacks->user_data);
                }
                return YMODEM_ERROR_CANCEL;
            }
            if (pres != 0) {
                retries++;
                if (retries > MAX_RETRIES) {
                    send_byte_rx(callbacks, XMODEM_CAN);
                    if (callbacks->on_file_end) {
                        callbacks->on_file_end(&info, YMODEM_ERROR_TIMEOUT, callbacks->user_data);
                    }
                    return YMODEM_ERROR_TIMEOUT;
                }
                send_byte_rx(callbacks, XMODEM_NAK);
                continue;
            }

            if (pkt_hdr == XMODEM_EOT) {
                /* NAK first EOT according to YMODEM specification */
                send_byte_rx(callbacks, XMODEM_NAK);

                uint8_t eot2;
                if (callbacks->read_byte(&eot2, PACKET_TIMEOUT_MS, callbacks->user_data) == 0 && eot2 == XMODEM_EOT) {
                    send_byte_rx(callbacks, XMODEM_ACK);
                    file_done = true;
                    if (callbacks->on_file_end) {
                        callbacks->on_file_end(&info, YMODEM_OK, callbacks->user_data);
                    }
                    break;
                } else {
                    send_byte_rx(callbacks, XMODEM_ACK);
                    file_done = true;
                    if (callbacks->on_file_end) {
                        callbacks->on_file_end(&info, YMODEM_OK, callbacks->user_data);
                    }
                    break;
                }
            }

            if (pkt_hdr == XMODEM_CAN) {
                if (callbacks->on_file_end) {
                    callbacks->on_file_end(&info, YMODEM_ERROR_CANCEL, callbacks->user_data);
                }
                return YMODEM_ERROR_CANCEL;
            }

            if (pkt_hdr != XMODEM_SOH && pkt_hdr != XMODEM_STX) {
                purge_rx(callbacks);
                send_byte_rx(callbacks, XMODEM_NAK);
                continue;
            }

            size_t blk_size = (pkt_hdr == XMODEM_STX) ? 1024 : 128;
            uint8_t bnum, bnum_inv;
            if (callbacks->read_byte(&bnum, BYTE_TIMEOUT_MS, callbacks->user_data) != 0 ||
                callbacks->read_byte(&bnum_inv, BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                purge_rx(callbacks);
                send_byte_rx(callbacks, XMODEM_NAK);
                continue;
            }

            bool io_err = false;
            for (size_t i = 0; i < blk_size; i++) {
                if (callbacks->read_byte(&payload[i], BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                    io_err = true;
                    break;
                }
            }
            uint8_t crc_h, crc_l;
            if (callbacks->read_byte(&crc_h, BYTE_TIMEOUT_MS, callbacks->user_data) != 0 ||
                callbacks->read_byte(&crc_l, BYTE_TIMEOUT_MS, callbacks->user_data) != 0) {
                io_err = true;
            }

            if (io_err) {
                purge_rx(callbacks);
                send_byte_rx(callbacks, XMODEM_NAK);
                continue;
            }

            uint16_t actual_crc = modem_crc16(payload, blk_size);
            uint16_t exp_crc = ((uint16_t)crc_h << 8) | crc_l;

            if (actual_crc != exp_crc) {
                purge_rx(callbacks);
                send_byte_rx(callbacks, XMODEM_NAK);
                continue;
            }

            if (bnum == expected_blk) {
                size_t write_len = blk_size;
                if (info.size > 0 && (file_offset + write_len) > info.size) {
                    write_len = info.size - file_offset;
                }

                if (callbacks->on_data && write_len > 0) {
                    callbacks->on_data(payload, write_len, file_offset, callbacks->user_data);
                }

                file_offset += blk_size;
                expected_blk++;
                retries = 0;
                send_byte_rx(callbacks, XMODEM_ACK);
            } else if (bnum == (uint8_t)(expected_blk - 1)) {
                send_byte_rx(callbacks, XMODEM_ACK);
            } else {
                send_byte_rx(callbacks, XMODEM_CAN);
                if (callbacks->on_file_end) {
                    callbacks->on_file_end(&info, YMODEM_ERROR_SEQUENCE, callbacks->user_data);
                }
                return YMODEM_ERROR_SEQUENCE;
            }
        }
    }
}

ymodem_status_t ymodem_transmit(const ymodem_tx_callbacks_t *callbacks)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes || !callbacks->get_file_info || !callbacks->read_data) {
        return YMODEM_ERROR_IO;
    }

    size_t file_idx = 0;

    while (1) {
        ymodem_file_info_t info;
        int has_file = callbacks->get_file_info(file_idx, &info, callbacks->user_data);

        uint8_t block0[128];
        memset(block0, 0, sizeof(block0));

        if (has_file == 0 && info.filename[0] != '\0') {
            size_t nlen = strlen(info.filename);
            if (nlen > 100) nlen = 100;
            memcpy(block0, info.filename, nlen);
            block0[nlen] = '\0';
            snprintf((char *)&block0[nlen + 1], sizeof(block0) - (nlen + 1), "%zu", info.size);
        }

        /* Wait for 'C' from receiver */
        uint8_t start_c;
        uint8_t retries = 0;
        bool got_c = false;

        while (retries < MAX_RETRIES && !got_c) {
            if (callbacks->read_byte(&start_c, PACKET_TIMEOUT_MS, callbacks->user_data) == 0) {
                if (start_c == XMODEM_C) {
                    got_c = true;
                    break;
                } else if (start_c == XMODEM_CAN) {
                    return YMODEM_ERROR_CANCEL;
                }
            }
            retries++;
        }

        if (!got_c) {
            return YMODEM_ERROR_TIMEOUT;
        }

        /* Send Block 0 (SOH, block 0, inv_block 0, 128 bytes, CRC16) */
        send_byte_tx(callbacks, XMODEM_SOH);
        send_byte_tx(callbacks, 0);
        send_byte_tx(callbacks, 0xFF);
        callbacks->write_bytes(block0, 128, callbacks->user_data);
        uint16_t crc0 = modem_crc16(block0, 128);
        uint8_t crc_buf[2] = { (uint8_t)(crc0 >> 8), (uint8_t)(crc0 & 0xFF) };
        callbacks->write_bytes(crc_buf, 2, callbacks->user_data);

        /* Wait ACK for Block 0 */
        uint8_t ack_resp;
        if (callbacks->read_byte(&ack_resp, PACKET_TIMEOUT_MS, callbacks->user_data) != 0 || ack_resp != XMODEM_ACK) {
            return YMODEM_ERROR_TIMEOUT;
        }

        if (has_file != 0 || info.filename[0] == '\0') {
            return YMODEM_OK;
        }

        /* Wait for second 'C' to start file data transfer */
        if (callbacks->read_byte(&start_c, PACKET_TIMEOUT_MS, callbacks->user_data) != 0 || start_c != XMODEM_C) {
            return YMODEM_ERROR_TIMEOUT;
        }

        /* Send file contents */
        size_t file_offset = 0;
        uint8_t block_num = 1;
        uint8_t payload[1024];

        while (file_offset < info.size) {
            size_t remaining = info.size - file_offset;
            size_t block_size = (remaining > 128) ? 1024 : 128;
            memset(payload, 0x1A, block_size);

            size_t read_bytes = (remaining > block_size) ? block_size : remaining;
            callbacks->read_data(file_idx, file_offset, payload, read_bytes, callbacks->user_data);

            retries = 0;
            bool acked = false;

            while (retries < MAX_RETRIES && !acked) {
                uint8_t hdr = (block_size == 1024) ? XMODEM_STX : XMODEM_SOH;
                send_byte_tx(callbacks, hdr);
                send_byte_tx(callbacks, block_num);
                send_byte_tx(callbacks, (uint8_t)~block_num);
                callbacks->write_bytes(payload, block_size, callbacks->user_data);

                uint16_t crc = modem_crc16(payload, block_size);
                uint8_t cbuf[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
                callbacks->write_bytes(cbuf, 2, callbacks->user_data);

                uint8_t resp;
                if (callbacks->read_byte(&resp, PACKET_TIMEOUT_MS, callbacks->user_data) == 0) {
                    if (resp == XMODEM_ACK) {
                        acked = true;
                    } else if (resp == XMODEM_CAN) {
                        return YMODEM_ERROR_CANCEL;
                    }
                }
                if (!acked) {
                    retries++;
                }
            }

            if (!acked) {
                return YMODEM_ERROR_TIMEOUT;
            }

            file_offset += read_bytes;
            block_num++;
        }

        /* Send EOT */
        send_byte_tx(callbacks, XMODEM_EOT);
        uint8_t eot_resp;
        if (callbacks->read_byte(&eot_resp, PACKET_TIMEOUT_MS, callbacks->user_data) == 0 && eot_resp == XMODEM_NAK) {
            send_byte_tx(callbacks, XMODEM_EOT);
            callbacks->read_byte(&eot_resp, PACKET_TIMEOUT_MS, callbacks->user_data);
        }

        file_idx++;
    }
}
