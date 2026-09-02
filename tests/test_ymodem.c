#include "modem/ymodem.h"
#include "modem/xmodem.h"
#include "modem/crc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FIFO_BUF_SIZE 4096

typedef struct {
    uint8_t rx_buf[FIFO_BUF_SIZE];
    size_t rx_head;
    size_t rx_tail;
} mock_pipe_t;

static void pipe_write(mock_pipe_t *pipe, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        pipe->rx_buf[pipe->rx_head] = data[i];
        pipe->rx_head = (pipe->rx_head + 1) % FIFO_BUF_SIZE;
    }
}

static int mock_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    mock_pipe_t *pipe = (mock_pipe_t *)user_data;
    if (pipe->rx_head == pipe->rx_tail) return -1;
    *byte = pipe->rx_buf[pipe->rx_tail];
    pipe->rx_tail = (pipe->rx_tail + 1) % FIFO_BUF_SIZE;
    return 0;
}

static int mock_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)buf;
    (void)len;
    (void)user_data;
    return 0;
}

static char last_file_name[256];
static size_t last_file_size = 0;
static uint8_t file_contents[1024];
static size_t file_bytes = 0;

static int mock_on_file_start(const ymodem_file_info_t *info, void *user_data)
{
    (void)user_data;
    strncpy(last_file_name, info->filename, sizeof(last_file_name));
    last_file_size = info->size;
    file_bytes = 0;
    return 0;
}

static int mock_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
    (void)user_data;
    (void)offset;
    memcpy(file_contents + file_bytes, buf, len);
    file_bytes += len;
    return 0;
}

static void mock_on_file_end(const ymodem_file_info_t *info, ymodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    (void)user_data;
}

void test_ymodem_receive_file(void)
{
    mock_pipe_t pipe = {0};

    /* Build Block 0: filename "test.txt\012" */
    uint8_t block0[128] = {0};
    strcpy((char *)block0, "test.txt");
    strcpy((char *)&block0[9], "12");

    uint8_t pkt0[133];
    pkt0[0] = XMODEM_SOH;
    pkt0[1] = 0;
    pkt0[2] = 0xFF;
    memcpy(&pkt0[3], block0, 128);
    uint16_t crc0 = modem_crc16(block0, 128);
    pkt0[131] = (uint8_t)(crc0 >> 8);
    pkt0[132] = (uint8_t)(crc0 & 0xFF);

    pipe_write(&pipe, pkt0, sizeof(pkt0));

    /* Build Block 1: "Hello World!" */
    uint8_t block1[128];
    memset(block1, 0x1A, 128);
    memcpy(block1, "Hello World!", 12);

    uint8_t pkt1[133];
    pkt1[0] = XMODEM_SOH;
    pkt1[1] = 1;
    pkt1[2] = 0xFE;
    memcpy(&pkt1[3], block1, 128);
    uint16_t crc1 = modem_crc16(block1, 128);
    pkt1[131] = (uint8_t)(crc1 >> 8);
    pkt1[132] = (uint8_t)(crc1 & 0xFF);

    pipe_write(&pipe, pkt1, sizeof(pkt1));

    /* EOT 1 & 2 */
    uint8_t eot = XMODEM_EOT;
    pipe_write(&pipe, &eot, 1);
    pipe_write(&pipe, &eot, 1);

    /* Empty Block 0 to end YMODEM batch */
    uint8_t empty_b0[128] = {0};
    uint8_t pkt_end[133];
    pkt_end[0] = XMODEM_SOH;
    pkt_end[1] = 0;
    pkt_end[2] = 0xFF;
    memcpy(&pkt_end[3], empty_b0, 128);
    uint16_t crc_end = modem_crc16(empty_b0, 128);
    pkt_end[131] = (uint8_t)(crc_end >> 8);
    pkt_end[132] = (uint8_t)(crc_end & 0xFF);

    pipe_write(&pipe, pkt_end, sizeof(pkt_end));

    ymodem_rx_callbacks_t cbs = {
        .read_byte = mock_read_byte,
        .write_bytes = mock_write_bytes,
        .on_file_start = mock_on_file_start,
        .on_data = mock_on_data,
        .on_file_end = mock_on_file_end,
        .user_data = &pipe
    };

    ymodem_status_t res = ymodem_receive(&cbs);
    assert(res == YMODEM_OK);
    assert(strcmp(last_file_name, "test.txt") == 0);
    assert(last_file_size == 12);
    assert(file_bytes == 12);
    assert(memcmp(file_contents, "Hello World!", 12) == 0);

    printf("[PASS] test_ymodem_receive_file\n");
}

int main(void)
{
    test_ymodem_receive_file();
    printf("All YMODEM tests passed!\n");
    return 0;
}
