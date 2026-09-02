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
