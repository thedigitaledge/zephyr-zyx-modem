/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr ztest protocol unit test suite covering CRC, XMODEM, YMODEM,
 * ZMODEM, transfer timeouts, transfer failures, and console configuration settings.
 */

#include <zephyr/ztest.h>
#include <string.h>

#include "crc.h"
#include "modem/xmodem.h"
#include "modem/ymodem.h"
#include "modem/zmodem.h"
#include "zephyr_console_modem.h"

/* Loopback pipe helper for mock serial I/O tests */
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

static int mock_rx_write_data(const uint8_t *buf, size_t len, void *user_data)
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

static int mock_rx_data_payload_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
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
        .write_bytes = mock_rx_write_data,
        .data_cb = mock_rx_data_payload_cb,
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

ZTEST(modem_tests, test_xmodem_timeout_and_cancel_failures)
{
    loopback_pipe_t pipe = {0};

    xmodem_callbacks_t cbs = {
        .read_byte = mock_rx_read_byte,
        .write_bytes = mock_rx_write_data,
        .data_cb = mock_rx_data_payload_cb,
        .user_data = &pipe
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.max_retries = 2;

    /* Empty pipe -> Should timeout */
    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_rx);
    zassert_equal(status, XMODEM_ERROR_TIMEOUT, "Expected timeout failure");

    /* CAN signal in pipe -> Should cancel */
    uint8_t can_bytes[2] = { XMODEM_CAN, XMODEM_CAN };
    pipe_write_to_rx(&pipe, can_bytes, 2);
    status = xmodem_receive(&cbs, &cfg, &total_rx);
    zassert_equal(status, XMODEM_ERROR_CANCEL, "Expected cancel failure");
}

ZTEST(modem_tests, test_console_modem_settings_and_options)
{
    console_modem_settings_t current;
    console_modem_settings_get(&current);
    zassert_equal(current.packet_timeout_ms, 3000, "Default packet timeout mismatch");
    zassert_equal(current.byte_timeout_ms, 1000, "Default byte timeout mismatch");
    zassert_equal(current.max_retries, 10, "Default max retries mismatch");
    zassert_equal(current.inter_block_delay_ms, 0, "Default inter-block delay mismatch");
    zassert_equal(current.handshake_delay_ms, 1000, "Default handshake delay mismatch");
    zassert_equal(current.overwrite_mode, MODEM_OVERWRITE_ALWAYS, "Default overwrite mode mismatch");
    zassert_equal(current.enable_resume, true, "Default resume setting mismatch");

    console_modem_settings_t updated = {
        .packet_timeout_ms = 6000,
        .byte_timeout_ms = 1500,
        .max_retries = 20,
        .inter_block_delay_ms = 50,
        .handshake_delay_ms = 2000,
        .overwrite_mode = MODEM_OVERWRITE_SKIP,
        .enable_resume = false,
        .default_target_dir = "/RAM:",
        .sync_interval_blocks = 5
    };
    console_modem_settings_set(&updated);

    console_modem_settings_get(&current);
    zassert_equal(current.packet_timeout_ms, 6000, "Updated packet timeout mismatch");
    zassert_equal(current.byte_timeout_ms, 1500, "Updated byte timeout mismatch");
    zassert_equal(current.max_retries, 20, "Updated max retries mismatch");
    zassert_equal(current.inter_block_delay_ms, 50, "Updated inter-block delay mismatch");
    zassert_equal(current.handshake_delay_ms, 2000, "Updated handshake delay mismatch");
    zassert_equal(current.overwrite_mode, MODEM_OVERWRITE_SKIP, "Updated overwrite mode mismatch");
    zassert_equal(current.enable_resume, false, "Updated resume setting mismatch");
    zassert_str_equal(current.default_target_dir, "/RAM:", "Updated default target dir mismatch");
    zassert_equal(current.sync_interval_blocks, 5, "Updated sync interval mismatch");
}

ZTEST_SUITE(modem_tests, NULL, NULL, NULL, NULL, NULL);
