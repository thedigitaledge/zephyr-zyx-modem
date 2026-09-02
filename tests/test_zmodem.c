#include "modem/zmodem.h"
#include "modem/crc.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FIFO_BUF_SIZE 8192

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

static int mock_on_file_start(const zmodem_file_info_t *info, void *user_data)
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

static void mock_on_file_end(const zmodem_file_info_t *info, zmodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    (void)user_data;
}

void test_zmodem_receive_sequence(void)
{
    mock_pipe_t pipe = {0};

    /* Build HEX ZFILE frame: "*\x18B0400000000" + CRC */
    uint8_t hbuf[5] = {ZFILE, 0, 0, 0, 0};
    uint16_t crc_h = modem_crc16(hbuf, 5);

    const char zfile_hdr[] = {'*', ZDLE, ZHEX, '0', '4', '0', '0', '0', '0', '0', '0', '\0'};
    pipe_write(&pipe, (const uint8_t *)zfile_hdr, 10);

    char crc_hex[5];
    snprintf(crc_hex, sizeof(crc_hex), "%04x", crc_h);
    pipe_write(&pipe, (const uint8_t *)crc_hex, 4);
    pipe_write(&pipe, (const uint8_t *)"\r\n\x11", 3);

    /* ZFILE payload: "sample.txt\05\0" + ZDLE ZCRCW + CRC */
    uint8_t zfile_payload[] = {'s','a','m','p','l','e','.','t','x','t','\0','5'};
    size_t plen = sizeof(zfile_payload);
    pipe_write(&pipe, zfile_payload, plen);

    uint8_t ender_seq[2] = {ZDLE, ZCRCW};
    pipe_write(&pipe, ender_seq, 2);

    uint16_t crc_p = modem_crc16(zfile_payload, plen);
    uint8_t ender = ZCRCW;
    crc_p = modem_crc16_update(crc_p, &ender, 1);
    uint8_t crc_p_bytes[2] = {(uint8_t)(crc_p >> 8), (uint8_t)(crc_p & 0xFF)};
    pipe_write(&pipe, crc_p_bytes, 2);

    /* ZDATA header */
    uint8_t zdata_hbuf[5] = {ZDATA, 0, 0, 0, 0};
    uint16_t crc_zdata = modem_crc16(zdata_hbuf, 5);
    const char zdata_hdr[] = {'*', ZDLE, ZHEX, '0', 'a', '0', '0', '0', '0', '0', '0', '\0'};
    pipe_write(&pipe, (const uint8_t *)zdata_hdr, 10);
    snprintf(crc_hex, sizeof(crc_hex), "%04x", crc_zdata);
    pipe_write(&pipe, (const uint8_t *)crc_hex, 4);
    pipe_write(&pipe, (const uint8_t *)"\r\n\x11", 3);

    /* ZDATA subpacket: "Hello" + ZDLE ZCRCW + CRC */
    uint8_t payload[] = "Hello";
    pipe_write(&pipe, payload, 5);
    pipe_write(&pipe, ender_seq, 2);

    uint16_t crc_d = modem_crc16(payload, 5);
    crc_d = modem_crc16_update(crc_d, &ender, 1);
    uint8_t crc_d_bytes[2] = {(uint8_t)(crc_d >> 8), (uint8_t)(crc_d & 0xFF)};
    pipe_write(&pipe, crc_d_bytes, 2);

    /* ZEOF header */
    uint8_t zeof_hbuf[5] = {ZEOF, 5, 0, 0, 0};
    uint16_t crc_zeof = modem_crc16(zeof_hbuf, 5);
    const char zeof_hdr[] = {'*', ZDLE, ZHEX, '0', 'b', '0', '5', '0', '0', '0', '0', '\0'};
    pipe_write(&pipe, (const uint8_t *)zeof_hdr, 10);
    snprintf(crc_hex, sizeof(crc_hex), "%04x", crc_zeof);
    pipe_write(&pipe, (const uint8_t *)crc_hex, 4);
    pipe_write(&pipe, (const uint8_t *)"\r\n\x11", 3);

    /* ZFIN header */
    uint8_t zfin_hbuf[5] = {ZFIN, 0, 0, 0, 0};
    uint16_t crc_zfin = modem_crc16(zfin_hbuf, 5);
    const char zfin_hdr[] = {'*', ZDLE, ZHEX, '0', '8', '0', '0', '0', '0', '0', '0', '\0'};
    pipe_write(&pipe, (const uint8_t *)zfin_hdr, 10);
    snprintf(crc_hex, sizeof(crc_hex), "%04x", crc_zfin);
    pipe_write(&pipe, (const uint8_t *)crc_hex, 4);
    pipe_write(&pipe, (const uint8_t *)"\r\n\x11", 3);

    zmodem_rx_callbacks_t cbs = {
        .read_byte = mock_read_byte,
        .write_bytes = mock_write_bytes,
        .on_file_start = mock_on_file_start,
        .on_data = mock_on_data,
        .on_file_end = mock_on_file_end,
        .user_data = &pipe
    };

    zmodem_status_t res = zmodem_receive(&cbs);
    assert(res == ZMODEM_OK);
    assert(strcmp(last_file_name, "sample.txt") == 0);
    assert(last_file_size == 5);
    assert(file_bytes == 5);
    assert(memcmp(file_contents, "Hello", 5) == 0);

    printf("[PASS] test_zmodem_receive_sequence\n");
}

int main(void)
{
    test_zmodem_receive_sequence();
    printf("All ZMODEM tests passed!\n");
    return 0;
}
