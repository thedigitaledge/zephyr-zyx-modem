#include <zephyr/ztest.h>
#include <string.h>

#include "modem/crc.h"
#include "modem/xmodem.h"
#include "modem/ymodem.h"
#include "modem/zmodem.h"

/* Loopback pipe helper */
#define FIFO_BUF_SIZE 4096

typedef struct {
    uint8_t rx_buf[FIFO_BUF_SIZE];
    size_t rx_head;
    size_t rx_tail;

    uint8_t tx_buf[FIFO_BUF_SIZE];
    size_t tx_head;
    size_t tx_tail;
} loopback_pipe_t;

static void pipe_write_to_rx(loopback_pipe_t *pipe, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        pipe->rx_buf[pipe->rx_head] = data[i];
        pipe->rx_head = (pipe->rx_head + 1) % FIFO_BUF_SIZE;
    }
}

static int mock_rx_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    loopback_pipe_t *pipe = (loopback_pipe_t *)user_data;
    if (pipe->rx_head == pipe->rx_tail) {
        return -1;
    }
    *byte = pipe->rx_buf[pipe->rx_tail];
    pipe->rx_tail = (pipe->rx_tail + 1) % FIFO_BUF_SIZE;
    return 0;
}

static int mock_rx_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    loopback_pipe_t *pipe = (loopback_pipe_t *)user_data;
    for (size_t i = 0; i < len; i++) {
        pipe->tx_buf[pipe->tx_head] = buf[i];
        pipe->tx_head = (pipe->tx_head + 1) % FIFO_BUF_SIZE;
    }
    return 0;
}

static uint8_t received_payload[XMODEM_BLOCK_SIZE_1024];
static size_t total_payload_received = 0;

static int mock_rx_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)block_num;
    (void)user_data;
    memcpy(received_payload + total_payload_received, buf, len);
    total_payload_received += len;
    return 0;
}

/* ZTEST Test Cases */

ZTEST(modem_tests, test_crc_service)
{
    const uint8_t data[] = "123456789";
    uint8_t cksum = modem_checksum8(data, 9);
    zassert_equal(cksum, 0xDD, "Checksum8 failed");

    uint16_t crc16 = modem_crc16(data, 9);
    zassert_equal(crc16, 0x29B1, "CRC16 CCITT failed");

    uint32_t crc32 = modem_crc32(data, 9);
    zassert_equal(crc32, 0xCBF43926U, "CRC32 IEEE failed");
}

ZTEST(modem_tests, test_xmodem_crc_receive)
{
    loopback_pipe_t pipe = {0};
    total_payload_received = 0;

    xmodem_callbacks_t cbs = {
        .read_byte = mock_rx_read_byte,
        .write_bytes = mock_rx_write_bytes,
        .data_cb = mock_rx_data_cb,
        .user_data = &pipe
    };

    uint8_t block[128];
    for (int i = 0; i < 128; i++) block[i] = (uint8_t)i;

    uint16_t crc = modem_crc16(block, 128);

    uint8_t pkt[1 + 2 + 128 + 2];
    pkt[0] = XMODEM_SOH;
    pkt[1] = 1;
    pkt[2] = 0xFE;
    memcpy(&pkt[3], block, 128);
    pkt[131] = (uint8_t)(crc >> 8);
    pkt[132] = (uint8_t)(crc & 0xFF);

    pipe_write_to_rx(&pipe, pkt, sizeof(pkt));
    uint8_t eot = XMODEM_EOT;
    pipe_write_to_rx(&pipe, &eot, 1);

    size_t total_rx = 0;
    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.mode = XMODEM_MODE_CRC;

    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_rx);
    zassert_equal(status, XMODEM_OK, "XMODEM receive failed");
    zassert_equal(total_rx, 128, "Total received size mismatch");
    zassert_memequal(received_payload, block, 128, "Payload mismatch");
}

ZTEST_SUITE(modem_tests, NULL, NULL, NULL, NULL, NULL);
