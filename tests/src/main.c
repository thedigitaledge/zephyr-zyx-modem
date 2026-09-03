/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Zephyr ztest protocol unit test suite covering CRC, XMODEM, YMODEM,
 * ZMODEM, BLE NUS, network sockets, stream decompression, delta patching,
 * signature verification, encrypted envelopes, and console configuration settings.
 */

#include <zephyr/ztest.h>
#include <string.h>

#include "crc.h"
#include "modem/xmodem.h"
#include "modem/ymodem.h"
#include "modem/zmodem.h"
#include "modem/ble_nus_transport.h"
#include "modem/net_socket_transport.h"
#include "modem/stream_decompress.h"
#include "modem/delta_update.h"
#include "modem/signature_verify.h"
#include "modem/encrypted_stream.h"
#include "modem/session_dispatcher.h"
#include "modem/log_rotation.h"
#include "modem/nfc_transport.h"
#include "zephyr_console_modem.h"
#include "mcuboot_validate.h"

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
    zassert_equal(crc16, 0x31C3, "CRC16 CCITT failed");

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
    zassert_mem_equal(received_payload, block, 128, "Payload mismatch");
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

static int mock_rx_read_byte_cancel(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)byte;
    (void)timeout_ms;
    (void)user_data;
    return -2; /* Immediate user cancel signal */
}

ZTEST(modem_tests, test_immediate_user_cancellation)
{
    loopback_pipe_t pipe = {0};

    xmodem_callbacks_t cbs = {
        .read_byte = mock_rx_read_byte_cancel,
        .write_bytes = mock_rx_write_data,
        .data_cb = mock_rx_data_payload_cb,
        .user_data = &pipe
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);

    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_rx);
    zassert_equal(status, XMODEM_ERROR_CANCEL, "Expected immediate user cancellation");
}

ZTEST(modem_tests, test_ble_nus_and_socket_transports)
{
    uint8_t b = 0;

#if defined(CONFIG_MODEM_BLE_NUS)
    ble_nus_transport_config_t nus_cfg = { .rx_ring_buffer_size = 512, .conn_timeout_ms = 1000 };
    int nus_init = ble_nus_transport_init(&nus_cfg);
    zassert_equal(nus_init, 0, "BLE NUS transport init failed");
    zassert_true(ble_nus_transport_is_connected(), "BLE NUS should report connected");

    uint8_t sample_rx[] = { 'A', 'B', 'C' };
    ble_nus_transport_rx_callback(sample_rx, 3);

    zassert_equal(ble_nus_transport_read_byte(&b, 10, NULL), 0, "BLE NUS read byte failed");
    zassert_equal(b, 'A', "BLE NUS read byte mismatch");
#endif

#if defined(CONFIG_MODEM_NET_SOCKET)
    net_socket_transport_config_t sock_cfg = { .socket_fd = 3, .read_timeout_ms = 1000 };
    int sock_init = net_socket_transport_init(&sock_cfg);
    zassert_equal(sock_init, 0, "Network socket transport init failed");
    zassert_equal(net_socket_transport_close(NULL), 0, "Socket close failed");
#endif

#if defined(CONFIG_MODEM_NFC)
    nfc_transport_config_t nfc_cfg = {
        .rx_buffer_size = 256,
        .tx_buffer_size = 256,
        .field_timeout_ms = 1000,
        .auto_ndef_framing = true
    };
    int nfc_init = nfc_transport_init(&nfc_cfg);
    zassert_equal(nfc_init, 0, "NFC transport init failed");
    zassert_equal(nfc_transport_start(), 0, "NFC transport start failed");
    zassert_true(nfc_transport_is_active(), "NFC field should be active after start");

    uint8_t nfc_sample[] = { 'N', 'F', 'C' };
    nfc_transport_rx_callback(nfc_sample, 3);
    zassert_equal(nfc_transport_read_byte(&b, 10, NULL), 0, "NFC read byte failed");
    zassert_equal(b, 'N', "NFC read byte mismatch");
    zassert_equal(nfc_transport_read_byte(&b, 10, NULL), 0, "NFC read byte 2 failed");
    zassert_equal(b, 'F', "NFC read byte 2 mismatch");
    zassert_equal(nfc_transport_read_byte(&b, 10, NULL), 0, "NFC read byte 3 failed");
    zassert_equal(b, 'C', "NFC read byte 3 mismatch");

    /* Test NDEF Record Encoding and Ring Buffer Flushing */
    uint8_t tx_payload[] = "NFC Data Payload";
    zassert_equal(nfc_transport_write_bytes(tx_payload, sizeof(tx_payload), NULL), 0, "NFC write bytes failed");

    uint8_t ndef_out[256];
    size_t ndef_len = 0;
    zassert_equal(nfc_transport_flush_tx_ndef(ndef_out, sizeof(ndef_out), &ndef_len), 0, "NFC flush NDEF failed");
    zassert_true(ndef_len > sizeof(tx_payload), "NDEF length should include header bytes");

    /* Test NDEF Record Decoding Callback */
    nfc_transport_rx_callback(ndef_out, ndef_len);
    uint8_t decoded_byte = 0;
    zassert_equal(nfc_transport_read_byte(&decoded_byte, 10, NULL), 0, "NFC decoded read byte failed");
    zassert_equal(decoded_byte, 'N', "NFC decoded byte mismatch");
    /* Drain remaining decoded payload bytes */
    while (nfc_transport_read_byte(&b, 1, NULL) == 0) {}

    /* Test Nordic T4T Event Handler Integration */
    zassert_equal(nfc_transport_start_t4t_emulation(), 0, "T4T emulation start failed");
    nfc_transport_t4t_event_handler(NFC_MODEM_EVENT_FIELD_ON, NULL, 0, NULL);
    zassert_true(nfc_transport_is_active(), "Field should be active on FIELD_ON event");

    uint8_t t4t_payload[] = "T4T Event Payload";
    nfc_transport_t4t_event_handler(NFC_MODEM_EVENT_NDEF_UPDATED, t4t_payload, sizeof(t4t_payload), NULL);

    uint8_t t4t_b = 0;
    zassert_equal(nfc_transport_read_byte(&t4t_b, 10, NULL), 0, "T4T event read byte failed");
    zassert_equal(t4t_b, 'T', "T4T event read byte mismatch");
    /* Drain remaining T4T payload bytes */
    while (nfc_transport_read_byte(&b, 1, NULL) == 0) {}

    nfc_transport_t4t_event_handler(NFC_MODEM_EVENT_FIELD_OFF, NULL, 0, NULL);
    zassert_false(nfc_transport_is_active(), "NFC field should be inactive after FIELD_OFF event");
    zassert_equal(nfc_transport_read_byte(&b, 10, NULL), -2, "NFC read byte on field loss should return -2");

    nfc_transport_stats_t nfc_stats;
    nfc_transport_get_stats(&nfc_stats);
    zassert_true(nfc_stats.field_loss_count > 0, "Field loss count stat should be incremented");
    zassert_true(nfc_stats.t4t_events_handled > 0, "T4T events handled stat should be incremented");
#endif
}

ZTEST(modem_tests, test_stream_decompress_and_delta_update)
{
    modem_decompress_ctx_t dctx;
    stream_decompress_init(&dctx, MODEM_COMPRESS_NONE);

    uint8_t raw_data[] = "Hello World!";
    uint8_t out_buf[32] = {0};
    size_t produced = 0;

    int res = stream_decompress_process(&dctx, raw_data, sizeof(raw_data), out_buf, sizeof(out_buf), &produced);
    zassert_equal(res, 0, "Decompress process failed");
    zassert_equal(produced, sizeof(raw_data), "Decompress produced len mismatch");
    zassert_mem_equal(out_buf, raw_data, sizeof(raw_data), "Decompressed payload mismatch");

    modem_delta_ctx_t delta_ctx;
    delta_update_init(&delta_ctx, 100);

    uint8_t diff[] = { 0x01, 0x02, 0x03 };
    uint8_t patch_out[16] = {0};
    size_t patch_len = 0;

    res = delta_update_apply_chunk(&delta_ctx, diff, sizeof(diff), NULL, patch_out, sizeof(patch_out), &patch_len);
    zassert_equal(res, 0, "Delta patch apply failed");
    zassert_equal(patch_len, 3, "Delta patch produced len mismatch");
    zassert_mem_equal(patch_out, diff, 3, "Delta patch content mismatch");
}

ZTEST(modem_tests, test_session_dispatcher_and_log_rotation)
{
    session_dispatcher_init();
    zassert_equal(session_dispatcher_get_active_count(), 0, "Initial active sessions should be 0");

    int id1 = session_dispatcher_create(MODEM_CHANNEL_UART);
    zassert_true(id1 >= 0, "Failed to create session 1");
    zassert_equal(session_dispatcher_get_active_count(), 1, "Active sessions should be 1");

    int id2 = session_dispatcher_create(MODEM_CHANNEL_BLE_NUS);
    zassert_true(id2 >= 0, "Failed to create session 2");
    zassert_equal(session_dispatcher_get_active_count(), 2, "Active sessions should be 2");

    int id3 = session_dispatcher_create(MODEM_CHANNEL_NFC);
    zassert_true(id3 >= 0, "Failed to create NFC session");
    zassert_equal(session_dispatcher_get_active_count(), 3, "Active sessions should be 3");

    session_dispatcher_close(id1);
    zassert_equal(session_dispatcher_get_active_count(), 2, "Active sessions after close should be 2");

    log_rotation_ctx_t log_ctx;
    log_rotation_config_t log_cfg = {
        .max_chunk_size = 10,
        .max_total_chunks = 3,
        .base_filename = "app_log"
    };
    log_rotation_init(&log_ctx, &log_cfg);

    char fname[64] = {0};
    uint8_t data[] = "12345";
    log_rotation_write(&log_ctx, data, sizeof(data), fname, sizeof(fname));
    zassert_str_equal(fname, "app_log_0.log", "Log chunk 0 filename mismatch");

    log_rotation_write(&log_ctx, data, 10, fname, sizeof(fname));
    zassert_str_equal(fname, "app_log_1.log", "Log chunk rotation filename mismatch");
}

ZTEST(modem_tests, test_signature_and_encryption)
{
    modem_sig_verify_ctx_t sig_ctx;
    signature_verify_init(&sig_ctx);

    uint8_t chunk[] = "Payload binary data";
    signature_verify_update(&sig_ctx, chunk, sizeof(chunk));

    uint8_t fake_sig[64] = {0};
    uint8_t fake_key[32] = {0};
    int sig_res = signature_verify_final(&sig_ctx, fake_sig, sizeof(fake_sig), fake_key, sizeof(fake_key));
    zassert_equal(sig_res, 0, "Signature verification final failed");

    modem_encrypted_stream_ctx_t enc_ctx;
    uint8_t key[32] = {0x01, 0x02, 0x03};
    encrypted_stream_init(&enc_ctx, key, sizeof(key));

    uint8_t plain[] = "Secret Payload";
    uint8_t cipher[32] = {0};
    uint8_t decrypted[32] = {0};
    size_t c_len = 0, d_len = 0;

    encrypted_stream_encrypt(&enc_ctx, plain, sizeof(plain), cipher, sizeof(cipher), &c_len);
    zassert_equal(c_len, sizeof(plain), "Encrypted length mismatch");

    encrypted_stream_decrypt(&enc_ctx, cipher, c_len, decrypted, sizeof(decrypted), &d_len);
    zassert_equal(d_len, sizeof(plain), "Decrypted length mismatch");
    zassert_mem_equal(decrypted, plain, sizeof(plain), "Decrypted payload mismatch");
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

    console_modem_settings_t updated;
    console_modem_settings_get(&updated);
    updated.packet_timeout_ms = 6000;
    updated.byte_timeout_ms = 1500;
    updated.max_retries = 20;
    updated.inter_block_delay_ms = 50;
    updated.handshake_delay_ms = 2000;
    updated.overwrite_mode = MODEM_OVERWRITE_SKIP;
#if defined(CONFIG_MODEM_ENABLE_RESUME)
    updated.enable_resume = false;
#endif
    strncpy(updated.default_target_dir, "/RAM:", sizeof(updated.default_target_dir) - 1);
    updated.sync_interval_blocks = 5;
#if defined(CONFIG_MODEM_SIGNATURE_VERIFY)
    updated.signature_verify = true;
#endif
#if defined(CONFIG_MODEM_ENCRYPTED_STREAM)
    updated.encrypted_envelope = true;
#endif
    console_modem_settings_set(&updated);

    console_modem_settings_get(&current);
    zassert_equal(current.packet_timeout_ms, 6000, "Updated packet timeout mismatch");
    zassert_equal(current.byte_timeout_ms, 1500, "Updated byte timeout mismatch");
    zassert_equal(current.max_retries, 20, "Updated max retries mismatch");
    zassert_equal(current.inter_block_delay_ms, 50, "Updated inter-block delay mismatch");
    zassert_equal(current.handshake_delay_ms, 2000, "Updated handshake delay mismatch");
    zassert_equal(current.overwrite_mode, MODEM_OVERWRITE_SKIP, "Updated overwrite mode mismatch");
#if defined(CONFIG_MODEM_ENABLE_RESUME)
    zassert_equal(current.enable_resume, false, "Updated resume setting mismatch");
#endif
    zassert_str_equal(current.default_target_dir, "/RAM:", "Updated default target dir mismatch");
    zassert_equal(current.sync_interval_blocks, 5, "Updated sync interval mismatch");
#if defined(CONFIG_MODEM_SIGNATURE_VERIFY)
    zassert_true(current.signature_verify, "Updated signature_verify mismatch");
#endif
#if defined(CONFIG_MODEM_ENCRYPTED_STREAM)
    zassert_true(current.encrypted_envelope, "Updated encrypted_envelope mismatch");
#endif
}

ZTEST(modem_tests, test_autostart_and_advanced_features)
{
#if defined(CONFIG_MODEM_AUTO_START)
    /* Test Autostart detection */
    zassert_false(console_modem_check_autostart('r'), "Autostart step 1 should return false");
    zassert_false(console_modem_check_autostart('z'), "Autostart step 2 should return false");
    zassert_true(console_modem_check_autostart('\r'), "Autostart trigger failed");
#endif

    console_modem_settings_t current;
    console_modem_settings_get(&current);
#if defined(CONFIG_MODEM_AUTO_START)
    zassert_true(current.auto_start, "Default auto_start setting mismatch");
#endif
#if defined(CONFIG_MODEM_ASYNC_STORAGE)
    zassert_true(current.async_storage, "Default async_storage setting mismatch");
#endif
#if defined(CONFIG_MODEM_PROGRESS_BAR)
    zassert_true(current.progress_bar, "Default progress_bar setting mismatch");
#endif
#if defined(CONFIG_MODEM_DIRECTORY_TRANSFERS)
    zassert_true(current.directory_transfers, "Default directory_transfers setting mismatch");
#endif
#if defined(CONFIG_MODEM_RING_BUFFER)
    zassert_true(current.ring_buffer, "Default ring_buffer setting mismatch");
#endif
#if defined(CONFIG_MODEM_ABORT_KEY)
    zassert_true(current.abort_key, "Default abort_key setting mismatch");
    zassert_equal(current.abort_key_char, 3, "Default abort_key_char setting mismatch");

    /* Test setting custom abort key character (e.g. 27 / ESC / 0x1B) */
    current.abort_key_char = 27;
    console_modem_settings_set(&current);

    console_modem_settings_get(&current);
    zassert_equal(current.abort_key_char, 27, "Updated abort_key_char setting mismatch");
#endif

#if defined(CONFIG_MODEM_FLOW_CONTROL)
    /* Test flow control toggle and channel binding API */
    current.flow_control = true;
    console_modem_settings_set(&current);
#endif

    console_modem_channel_t ch = {
        .uart_dev = NULL,
        .read_byte = mock_rx_read_byte,
        .write_bytes = mock_rx_write_data,
        .user_data = NULL
    };
    int bind_res = console_modem_bind_device(&ch);
    zassert_equal(bind_res, 0, "Device channel binding failed");

    /* Test transfer statistics counters */
    modem_stats_t stats;
    console_modem_stats_get(&stats);
    zassert_equal(stats.crc_errors, 0, "Stats CRC error count mismatch");
    console_modem_stats_reset();
    console_modem_stats_get(&stats);
    zassert_equal(stats.total_transfers, 0, "Stats reset failed");

    /* Test MCUBoot Image Header Validation */
    mcuboot_image_header_t hdr = {
        .magic = MCUBOOT_IMAGE_MAGIC,
        .img_size = 1024
    };
    int val_res = mcuboot_validate_header((const uint8_t *)&hdr, sizeof(hdr), 2048);
    zassert_equal(val_res, 0, "MCUBoot header validation failed for valid header");

    hdr.magic = 0x12345678;
    val_res = mcuboot_validate_header((const uint8_t *)&hdr, sizeof(hdr), 2048);
    zassert_equal(val_res, -2, "MCUBoot header validation failed to reject bad magic");
}

ZTEST(modem_tests, test_fault_injection_stress_harness)
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
    cfg.max_retries = 3;

    /* Inject corrupted block header to test retry handling */
    uint8_t corrupt_hdr[5] = { 0x99, 0xFF, 0x00, 0x11, 0x22 };
    pipe_write_to_rx(&pipe, corrupt_hdr, sizeof(corrupt_hdr));

    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_rx);
    zassert_equal(status, XMODEM_ERROR_TIMEOUT, "Expected failure on corrupted data pipe");
}

ZTEST_SUITE(modem_tests, NULL, NULL, NULL, NULL, NULL);
