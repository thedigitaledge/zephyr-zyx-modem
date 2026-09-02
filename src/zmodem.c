#include "modem/zmodem.h"
#include "modem/crc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ZMODEM_TIMEOUT_MS 5000

static const char hex_digits[] = "0123456789abcdef";

static int hex_to_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int send_byte_rx(const zmodem_rx_callbacks_t *cbs, uint8_t b)
{
    return cbs->write_bytes(&b, 1, cbs->user_data);
}

static int send_byte_tx(const zmodem_tx_callbacks_t *cbs, uint8_t b)
{
    return cbs->write_bytes(&b, 1, cbs->user_data);
}

static void send_hex_header(const zmodem_rx_callbacks_t *cbs, uint8_t type, const uint8_t flags[4])
{
    uint8_t buf[20];
    int pos = 0;
    buf[pos++] = ZPAD;
    buf[pos++] = ZPAD;
    buf[pos++] = ZDLE;
    buf[pos++] = ZHEX;

    uint8_t hbuf[5];
    hbuf[0] = type;
    hbuf[1] = flags[0];
    hbuf[2] = flags[1];
    hbuf[3] = flags[2];
    hbuf[4] = flags[3];

    for (int i = 0; i < 5; i++) {
        buf[pos++] = hex_digits[(hbuf[i] >> 4) & 0x0F];
        buf[pos++] = hex_digits[hbuf[i] & 0x0F];
    }

    uint16_t crc = modem_crc16(hbuf, 5);
    uint8_t crc_bytes[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
    for (int i = 0; i < 2; i++) {
        buf[pos++] = hex_digits[(crc_bytes[i] >> 4) & 0x0F];
        buf[pos++] = hex_digits[crc_bytes[i] & 0x0F];
    }

    buf[pos++] = '\r';
    buf[pos++] = '\n';
    if (type != ZFIN && type != ZACK) {
        buf[pos++] = 0x11; /* XON */
    }

    cbs->write_bytes(buf, pos, cbs->user_data);
}

static void send_hex_header_tx(const zmodem_tx_callbacks_t *cbs, uint8_t type, const uint8_t flags[4])
{
    uint8_t buf[20];
    int pos = 0;
    buf[pos++] = ZPAD;
    buf[pos++] = ZPAD;
    buf[pos++] = ZDLE;
    buf[pos++] = ZHEX;

    uint8_t hbuf[5];
    hbuf[0] = type;
    hbuf[1] = flags[0];
    hbuf[2] = flags[1];
    hbuf[3] = flags[2];
    hbuf[4] = flags[3];

    for (int i = 0; i < 5; i++) {
        buf[pos++] = hex_digits[(hbuf[i] >> 4) & 0x0F];
        buf[pos++] = hex_digits[hbuf[i] & 0x0F];
    }

    uint16_t crc = modem_crc16(hbuf, 5);
    uint8_t crc_bytes[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
    for (int i = 0; i < 2; i++) {
        buf[pos++] = hex_digits[(crc_bytes[i] >> 4) & 0x0F];
        buf[pos++] = hex_digits[crc_bytes[i] & 0x0F];
    }

    buf[pos++] = '\r';
    buf[pos++] = '\n';
    if (type != ZFIN && type != ZACK) {
        buf[pos++] = 0x11; /* XON */
    }

    cbs->write_bytes(buf, pos, cbs->user_data);
}

static int read_zdle_byte_rx(const zmodem_rx_callbacks_t *cbs, uint8_t *b)
{
    uint8_t ch;
    if (cbs->read_byte(&ch, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) {
        return -1;
    }
    if (ch == ZDLE) {
        if (cbs->read_byte(&ch, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) {
            return -1;
        }
        if (ch == ZCRCE || ch == ZCRCG || ch == ZCRCQ || ch == ZCRCW) {
            *b = ch;
            return 1; /* Special frame ender */
        }
        if (ch == ZDLEE) {
            *b = ZDLE;
            return 0;
        }
        *b = ch ^ 0x40;
        return 0;
    }
    *b = ch;
    return 0;
}

static int read_zdle_byte_tx(const zmodem_tx_callbacks_t *cbs, uint8_t *b)
{
    uint8_t ch;
    if (cbs->read_byte(&ch, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) {
        return -1;
    }
    if (ch == ZDLE) {
        if (cbs->read_byte(&ch, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) {
            return -1;
        }
        if (ch == ZCRCE || ch == ZCRCG || ch == ZCRCQ || ch == ZCRCW) {
            *b = ch;
            return 1;
        }
        if (ch == ZDLEE) {
            *b = ZDLE;
            return 0;
        }
        *b = ch ^ 0x40;
        return 0;
    }
    *b = ch;
    return 0;
}

static int read_header_rx(const zmodem_rx_callbacks_t *cbs, uint8_t *type, uint8_t flags[4])
{
    uint8_t b;
    while (1) {
        if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
        if (b == ZPAD) {
            if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
            if (b == ZPAD || b == ZDLE) {
                if (b == ZPAD) {
                    if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                }
                if (b == ZDLE) {
                    uint8_t format;
                    if (cbs->read_byte(&format, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                    if (format == ZHEX) {
                        uint8_t hexbuf[14];
                        for (int i = 0; i < 14; i++) {
                            if (cbs->read_byte(&hexbuf[i], ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                        }
                        int t_hi = hex_to_nibble((char)hexbuf[0]);
                        int t_lo = hex_to_nibble((char)hexbuf[1]);
                        if (t_hi < 0 || t_lo < 0) return -1;
                        *type = (t_hi << 4) | t_lo;
                        for (int i = 0; i < 4; i++) {
                            int f_hi = hex_to_nibble((char)hexbuf[2 + i * 2]);
                            int f_lo = hex_to_nibble((char)hexbuf[3 + i * 2]);
                            if (f_hi < 0 || f_lo < 0) return -1;
                            flags[i] = (f_hi << 4) | f_lo;
                        }
                        return 0;
                    } else if (format == ZBIN) {
                        uint8_t hbuf[5];
                        for (int i = 0; i < 5; i++) {
                            if (read_zdle_byte_rx(cbs, &hbuf[i]) < 0) return -1;
                        }
                        *type = hbuf[0];
                        memcpy(flags, &hbuf[1], 4);
                        /* Read 2 CRC bytes */
                        uint8_t c1, c2;
                        if (read_zdle_byte_rx(cbs, &c1) < 0 || read_zdle_byte_rx(cbs, &c2) < 0) return -1;
                        return 0;
                    }
                }
            }
        }
    }
}

static int read_header_tx(const zmodem_tx_callbacks_t *cbs, uint8_t *type, uint8_t flags[4])
{
    uint8_t b;
    while (1) {
        if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
        if (b == ZPAD) {
            if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
            if (b == ZPAD || b == ZDLE) {
                if (b == ZPAD) {
                    if (cbs->read_byte(&b, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                }
                if (b == ZDLE) {
                    uint8_t format;
                    if (cbs->read_byte(&format, ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                    if (format == ZHEX) {
                        uint8_t hexbuf[14];
                        for (int i = 0; i < 14; i++) {
                            if (cbs->read_byte(&hexbuf[i], ZMODEM_TIMEOUT_MS, cbs->user_data) != 0) return -1;
                        }
                        int t_hi = hex_to_nibble((char)hexbuf[0]);
                        int t_lo = hex_to_nibble((char)hexbuf[1]);
                        if (t_hi < 0 || t_lo < 0) return -1;
                        *type = (t_hi << 4) | t_lo;
                        for (int i = 0; i < 4; i++) {
                            int f_hi = hex_to_nibble((char)hexbuf[2 + i * 2]);
                            int f_lo = hex_to_nibble((char)hexbuf[3 + i * 2]);
                            if (f_hi < 0 || f_lo < 0) return -1;
                            flags[i] = (f_hi << 4) | f_lo;
                        }
                        return 0;
                    } else if (format == ZBIN) {
                        uint8_t hbuf[5];
                        for (int i = 0; i < 5; i++) {
                            if (read_zdle_byte_tx(cbs, &hbuf[i]) < 0) return -1;
                        }
                        *type = hbuf[0];
                        memcpy(flags, &hbuf[1], 4);
                        uint8_t c1, c2;
                        if (read_zdle_byte_tx(cbs, &c1) < 0 || read_zdle_byte_tx(cbs, &c2) < 0) return -1;
                        return 0;
                    }
                }
            }
        }
    }
}

static int read_subpacket_rx(const zmodem_rx_callbacks_t *cbs, uint8_t *buf, size_t max_len, size_t *rx_len, uint8_t *ender)
{
    size_t pos = 0;
    while (1) {
        uint8_t b;
        int res = read_zdle_byte_rx(cbs, &b);
        if (res < 0) return -1;
        if (res == 1) {
            *ender = b;
            *rx_len = pos;
            /* Read 2 CRC bytes */
            uint8_t c1, c2;
            if (read_zdle_byte_rx(cbs, &c1) < 0 || read_zdle_byte_rx(cbs, &c2) < 0) return -1;
            return 0;
        }
        if (pos < max_len) {
            buf[pos++] = b;
        }
    }
}

zmodem_status_t zmodem_receive(const zmodem_rx_callbacks_t *callbacks)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes) {
        return ZMODEM_ERROR_IO;
    }

    uint8_t zero_flags[4] = {0, 0, 0, 0};

    /* Initiate session by sending ZRINIT */
    send_hex_header(callbacks, ZRINIT, zero_flags);

    while (1) {
        uint8_t type;
        uint8_t flags[4];
        if (read_header_rx(callbacks, &type, flags) != 0) {
            return ZMODEM_ERROR_TIMEOUT;
        }

        if (type == ZRQINIT) {
            send_hex_header(callbacks, ZRINIT, zero_flags);
            continue;
        }

        if (type == ZFILE) {
            uint8_t sub_buf[1024];
            size_t sub_len = 0;
            uint8_t ender;
            if (read_subpacket_rx(callbacks, sub_buf, sizeof(sub_buf), &sub_len, &ender) != 0) {
                return ZMODEM_ERROR_IO;
            }

            zmodem_file_info_t info;
            memset(&info, 0, sizeof(info));
            size_t nlen = strnlen((const char *)sub_buf, sub_len);
            if (nlen >= sizeof(info.filename)) nlen = sizeof(info.filename) - 1;
            memcpy(info.filename, sub_buf, nlen);
            info.filename[nlen] = '\0';

            if (nlen + 1 < sub_len) {
                info.size = (size_t)strtoul((const char *)&sub_buf[nlen + 1], NULL, 10);
            }

            if (callbacks->on_file_start) {
                callbacks->on_file_start(&info, callbacks->user_data);
            }

            uint8_t rpos_flags[4] = {0, 0, 0, 0};
            send_hex_header(callbacks, ZRPOS, rpos_flags);

            /* Receive ZDATA file blocks */
            size_t file_offset = 0;
            while (1) {
                if (read_header_rx(callbacks, &type, flags) != 0) {
                    if (callbacks->on_file_end) callbacks->on_file_end(&info, ZMODEM_ERROR_TIMEOUT, callbacks->user_data);
                    return ZMODEM_ERROR_TIMEOUT;
                }

                if (type == ZDATA) {
                    bool data_done = false;
                    while (!data_done) {
                        if (read_subpacket_rx(callbacks, sub_buf, sizeof(sub_buf), &sub_len, &ender) != 0) {
                            if (callbacks->on_file_end) callbacks->on_file_end(&info, ZMODEM_ERROR_IO, callbacks->user_data);
                            return ZMODEM_ERROR_IO;
                        }

                        if (callbacks->on_data && sub_len > 0) {
                            callbacks->on_data(sub_buf, sub_len, file_offset, callbacks->user_data);
                        }
                        file_offset += sub_len;

                        if (ender == ZCRCE || ender == ZCRCW) {
                            data_done = true;
                        }
                    }
                } else if (type == ZEOF) {
                    if (callbacks->on_file_end) {
                        callbacks->on_file_end(&info, ZMODEM_OK, callbacks->user_data);
                    }
                    send_hex_header(callbacks, ZRINIT, zero_flags);
                    break;
                } else if (type == ZFIN) {
                    send_hex_header(callbacks, ZFIN, zero_flags);
                    return ZMODEM_OK;
                }
            }
        } else if (type == ZFIN) {
            send_hex_header(callbacks, ZFIN, zero_flags);
            /* Send 'O', 'O' */
            send_byte_rx(callbacks, 'O');
            send_byte_rx(callbacks, 'O');
            return ZMODEM_OK;
        }
    }
}

static void send_zdle_byte_tx(const zmodem_tx_callbacks_t *cbs, uint8_t b)
{
    if (b == ZDLE || b == 0x11 || b == 0x13 || b == 0x81 || b == 0x83) {
        uint8_t esc[2] = { ZDLE, b ^ 0x40 };
        cbs->write_bytes(esc, 2, cbs->user_data);
    } else {
        cbs->write_bytes(&b, 1, cbs->user_data);
    }
}

static void send_binary_header_tx(const zmodem_tx_callbacks_t *cbs, uint8_t type, const uint8_t flags[4])
{
    uint8_t prefix[4] = { ZPAD, ZDLE, ZBIN };
    cbs->write_bytes(prefix, 3, cbs->user_data);

    send_zdle_byte_tx(cbs, type);
    for (int i = 0; i < 4; i++) {
        send_zdle_byte_tx(cbs, flags[i]);
    }

    uint8_t hbuf[5] = { type, flags[0], flags[1], flags[2], flags[3] };
    uint16_t crc = modem_crc16(hbuf, 5);
    send_zdle_byte_tx(cbs, (uint8_t)(crc >> 8));
    send_zdle_byte_tx(cbs, (uint8_t)(crc & 0xFF));
}

static void send_subpacket_tx(const zmodem_tx_callbacks_t *cbs, const uint8_t *buf, size_t len, uint8_t ender)
{
    for (size_t i = 0; i < len; i++) {
        send_zdle_byte_tx(cbs, buf[i]);
    }
    uint8_t end_seq[2] = { ZDLE, ender };
    cbs->write_bytes(end_seq, 2, cbs->user_data);

    uint16_t crc = modem_crc16(buf, len);
    crc = modem_crc16_update(crc, &ender, 1);
    send_zdle_byte_tx(cbs, (uint8_t)(crc >> 8));
    send_zdle_byte_tx(cbs, (uint8_t)(crc & 0xFF));
}

zmodem_status_t zmodem_transmit(const zmodem_tx_callbacks_t *callbacks)
{
    if (!callbacks || !callbacks->read_byte || !callbacks->write_bytes || !callbacks->get_file_info || !callbacks->read_data) {
        return ZMODEM_ERROR_IO;
    }

    uint8_t zero_flags[4] = {0, 0, 0, 0};

    /* Send ZRQINIT */
    send_hex_header_tx(callbacks, ZRQINIT, zero_flags);

    uint8_t type;
    uint8_t flags[4];

    /* Wait for ZRINIT */
    if (read_header_tx(callbacks, &type, flags) != 0 || type != ZRINIT) {
        return ZMODEM_ERROR_TIMEOUT;
    }

    size_t file_idx = 0;
    while (1) {
        zmodem_file_info_t info;
        int has_file = callbacks->get_file_info(file_idx, &info, callbacks->user_data);

        if (has_file != 0 || info.filename[0] == '\0') {
            /* No more files -> Send ZFIN */
            send_hex_header_tx(callbacks, ZFIN, zero_flags);
            if (read_header_tx(callbacks, &type, flags) == 0 && type == ZFIN) {
                send_byte_tx(callbacks, 'O');
                send_byte_tx(callbacks, 'O');
            }
            return ZMODEM_OK;
        }

        /* Send ZFILE frame with payload */
        send_binary_header_tx(callbacks, ZFILE, zero_flags);

        uint8_t sub_buf[512];
        memset(sub_buf, 0, sizeof(sub_buf));
        size_t nlen = strlen(info.filename);
        if (nlen > 200) nlen = 200;
        memcpy(sub_buf, info.filename, nlen);
        sub_buf[nlen] = '\0';
        snprintf((char *)&sub_buf[nlen + 1], sizeof(sub_buf) - (nlen + 1), "%zu", info.size);

        send_subpacket_tx(callbacks, sub_buf, nlen + 1 + strlen((char *)&sub_buf[nlen + 1]) + 1, ZCRCW);

        /* Wait for ZRPOS */
        if (read_header_tx(callbacks, &type, flags) != 0 || type != ZRPOS) {
            return ZMODEM_ERROR_TIMEOUT;
        }

        /* Send ZDATA */
        send_binary_header_tx(callbacks, ZDATA, zero_flags);

        size_t file_offset = 0;
        uint8_t data_buf[1024];

        while (file_offset < info.size) {
            size_t chunk = info.size - file_offset;
            if (chunk > sizeof(data_buf)) chunk = sizeof(data_buf);

            callbacks->read_data(file_idx, file_offset, data_buf, chunk, callbacks->user_data);
            file_offset += chunk;

            uint8_t ender = (file_offset >= info.size) ? ZCRCW : ZCRCG;
            send_subpacket_tx(callbacks, data_buf, chunk, ender);
        }

        /* Send ZEOF */
        uint8_t pos_flags[4] = {
            (uint8_t)(info.size & 0xFF),
            (uint8_t)((info.size >> 8) & 0xFF),
            (uint8_t)((info.size >> 16) & 0xFF),
            (uint8_t)((info.size >> 24) & 0xFF)
        };
        send_hex_header_tx(callbacks, ZEOF, pos_flags);

        /* Wait for ZRINIT */
        if (read_header_tx(callbacks, &type, flags) != 0 || type != ZRINIT) {
            return ZMODEM_ERROR_TIMEOUT;
        }

        file_idx++;
    }
}
