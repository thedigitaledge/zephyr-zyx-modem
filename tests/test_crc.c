#include "modem/crc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_checksum8(void)
{
    const uint8_t data[] = "123456789";
    uint8_t cksum = modem_checksum8(data, 9);
    /* sum = '1'+'2'+'3'+'4'+'5'+'6'+'7'+'8'+'9' = 49*9 + 36 = 477 % 256 = 221 (0xDD) */
    assert(cksum == 0xDD);
    printf("[PASS] test_checksum8\n");
}

void test_crc16_ccitt(void)
{
    const uint8_t data[] = "123456789";
    uint16_t crc = modem_crc16(data, 9);
    assert(crc == 0x31C3);
    printf("[PASS] test_crc16_ccitt\n");
}

void test_crc32_ieee(void)
{
    const uint8_t data[] = "123456789";
    uint32_t crc = modem_crc32(data, 9);
    assert(crc == 0xCBF43926U);
    printf("[PASS] test_crc32_ieee\n");
}

int main(void)
{
    test_checksum8();
    test_crc16_ccitt();
    test_crc32_ieee();
    printf("All CRC tests passed!\n");
    return 0;
}
