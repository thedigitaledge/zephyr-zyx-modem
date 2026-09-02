#include "modem/crc.h"

uint8_t modem_checksum8(const uint8_t *buf, size_t len)
{
    uint8_t sum = 0;
    if (!buf) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}

uint16_t modem_crc16_update(uint16_t crc, const uint8_t *buf, size_t len)
{
    if (!buf) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

uint16_t modem_crc16(const uint8_t *buf, size_t len)
{
    return modem_crc16_update(0x0000, buf, len);
}

uint32_t modem_crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    if (!buf) {
        return crc;
    }
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint32_t modem_crc32(const uint8_t *buf, size_t len)
{
    uint32_t crc = modem_crc32_update(0xFFFFFFFFU, buf, len);
    return modem_crc32_finalize(crc);
}
