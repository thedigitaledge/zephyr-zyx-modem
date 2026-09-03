/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Zephyr Console / Shell Serial Modem Protocol Adapter.
 * Integrates XMODEM, YMODEM, and ZMODEM file transfer routines
 * with Zephyr shell commands and Zephyr VFS file systems.
 */

#include "zephyr_console_modem.h"
#include "crc.h"

#if defined(CONFIG_MODEM_XMODEM)
#include "modem/xmodem.h"
#endif

#if defined(CONFIG_MODEM_YMODEM)
#include "modem/ymodem.h"
#endif

#if defined(CONFIG_MODEM_ZMODEM)
#include "modem/zmodem.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/shell/shell.h>
#include <zephyr/console/console.h>
#if defined(CONFIG_FILE_SYSTEM)
#include <zephyr/fs/fs.h>
#endif

#if !defined(CONFIG_MODEM_PACKET_TIMEOUT_MS)
#define CONFIG_MODEM_PACKET_TIMEOUT_MS 3000
#endif

#if !defined(CONFIG_MODEM_BYTE_TIMEOUT_MS)
#define CONFIG_MODEM_BYTE_TIMEOUT_MS 1000
#endif

#if !defined(CONFIG_MODEM_MAX_RETRIES)
#define CONFIG_MODEM_MAX_RETRIES 10
#endif

#if !defined(CONFIG_MODEM_INTER_BLOCK_DELAY_MS)
#define CONFIG_MODEM_INTER_BLOCK_DELAY_MS 0
#endif

#if !defined(CONFIG_MODEM_HANDSHAKE_DELAY_MS)
#define CONFIG_MODEM_HANDSHAKE_DELAY_MS 1000
#endif

#if !defined(CONFIG_MODEM_FILE_OVERWRITE_MODE)
#define CONFIG_MODEM_FILE_OVERWRITE_MODE 0
#endif

#if !defined(CONFIG_MODEM_DEFAULT_TARGET_DIR)
#define CONFIG_MODEM_DEFAULT_TARGET_DIR ""
#endif

#if !defined(CONFIG_MODEM_SYNC_INTERVAL_BLOCKS)
#define CONFIG_MODEM_SYNC_INTERVAL_BLOCKS 10
#endif

#if !defined(CONFIG_MODEM_ABORT_KEY_CHAR)
#define CONFIG_MODEM_ABORT_KEY_CHAR 3
#endif

/* Runtime Modem Configuration */
static console_modem_settings_t g_modem_settings = {
    .packet_timeout_ms = CONFIG_MODEM_PACKET_TIMEOUT_MS,
    .byte_timeout_ms = CONFIG_MODEM_BYTE_TIMEOUT_MS,
    .max_retries = CONFIG_MODEM_MAX_RETRIES,
    .inter_block_delay_ms = CONFIG_MODEM_INTER_BLOCK_DELAY_MS,
    .handshake_delay_ms = CONFIG_MODEM_HANDSHAKE_DELAY_MS,
    .overwrite_mode = (modem_overwrite_mode_t)CONFIG_MODEM_FILE_OVERWRITE_MODE,
#if defined(CONFIG_MODEM_ENABLE_RESUME)
    .enable_resume = IS_ENABLED(CONFIG_MODEM_ENABLE_RESUME),
#endif
    .default_target_dir = CONFIG_MODEM_DEFAULT_TARGET_DIR,
    .sync_interval_blocks = CONFIG_MODEM_SYNC_INTERVAL_BLOCKS,
#if defined(CONFIG_MODEM_AUTO_START)
    .auto_start = IS_ENABLED(CONFIG_MODEM_AUTO_START),
#endif
#if defined(CONFIG_MODEM_ASYNC_STORAGE)
    .async_storage = IS_ENABLED(CONFIG_MODEM_ASYNC_STORAGE),
#endif
#if defined(CONFIG_MODEM_PROGRESS_BAR)
    .progress_bar = IS_ENABLED(CONFIG_MODEM_PROGRESS_BAR),
#endif
#if defined(CONFIG_MODEM_DIRECTORY_TRANSFERS)
    .directory_transfers = IS_ENABLED(CONFIG_MODEM_DIRECTORY_TRANSFERS),
#endif
#if defined(CONFIG_MODEM_RING_BUFFER)
    .ring_buffer = IS_ENABLED(CONFIG_MODEM_RING_BUFFER),
#endif
#if defined(CONFIG_MODEM_ABORT_KEY)
    .abort_key = IS_ENABLED(CONFIG_MODEM_ABORT_KEY),
    .abort_key_char = CONFIG_MODEM_ABORT_KEY_CHAR,
#endif
#if defined(CONFIG_MODEM_FLOW_CONTROL)
    .flow_control = IS_ENABLED(CONFIG_MODEM_FLOW_CONTROL),
#endif
#if defined(CONFIG_MODEM_FLASH_PARTITION)
    .flash_partition = "",
#endif
#if defined(CONFIG_MODEM_MCUBOOT_UPDATE)
    .mcuboot_update = IS_ENABLED(CONFIG_MODEM_MCUBOOT_UPDATE),
#endif
#if defined(CONFIG_MODEM_NVS_CHECKPOINTS)
    .nvs_checkpoints = IS_ENABLED(CONFIG_MODEM_NVS_CHECKPOINTS),
#endif
#if defined(CONFIG_MODEM_CRYPTO_STREAM)
    .crypto_stream = IS_ENABLED(CONFIG_MODEM_CRYPTO_STREAM),
#endif
#if defined(CONFIG_MODEM_UART_DMA)
    .uart_dma = IS_ENABLED(CONFIG_MODEM_UART_DMA),
#endif
#if defined(CONFIG_MODEM_USB_CDC_ACM)
    .usb_cdc_acm = IS_ENABLED(CONFIG_MODEM_USB_CDC_ACM),
#endif
#if defined(CONFIG_MODEM_MCUBOOT_VALIDATE)
    .mcuboot_validate = IS_ENABLED(CONFIG_MODEM_MCUBOOT_VALIDATE),
#endif
#if defined(CONFIG_MODEM_SIGNATURE_VERIFY)
    .signature_verify = IS_ENABLED(CONFIG_MODEM_SIGNATURE_VERIFY),
#endif
#if defined(CONFIG_MODEM_ENCRYPTED_STREAM)
    .encrypted_envelope = IS_ENABLED(CONFIG_MODEM_ENCRYPTED_STREAM),
#endif
#if defined(CONFIG_MODEM_SESSION_DISPATCHER)
    .session_dispatcher = IS_ENABLED(CONFIG_MODEM_SESSION_DISPATCHER),
#endif
#if defined(CONFIG_MODEM_LOG_ROTATION)
    .log_rotation = IS_ENABLED(CONFIG_MODEM_LOG_ROTATION),
#endif
#if defined(CONFIG_MODEM_NFC)
    .nfc_transport = IS_ENABLED(CONFIG_MODEM_NFC)
#endif
};

static console_modem_channel_t *g_active_channel = NULL;

int console_modem_bind_device(console_modem_channel_t *channel)
{
    g_active_channel = channel;
    return 0;
}

void console_modem_settings_get(console_modem_settings_t *settings)
{
    if (settings) {
        *settings = g_modem_settings;
    }
}

void console_modem_settings_set(const console_modem_settings_t *settings)
{
    if (settings) {
        g_modem_settings = *settings;
    }
}

/**
 * Context structure managing current console transfer state and file handle.
 */
typedef struct {
    const void *shell_ctx;
#if defined(CONFIG_FILE_SYSTEM)
    struct fs_file_t zfile;
    bool zfile_open;
#endif
    size_t file_size;
    size_t bytes_transferred;
    int64_t start_time_ms;
    char target_path[256];
} console_modem_ctx_t;

static int g_can_count = 0;

/* Ring buffer UART transport adapter */
RING_BUF_DECLARE(g_uart_ring_buf, 512);

#if defined(CONFIG_FILE_SYSTEM)
/* Async storage work item structure */
struct async_storage_work_t {
    struct k_work work_item;
    struct fs_file_t *zfile;
    uint8_t data[1024];
    size_t len;
    ssize_t res;
};

static struct async_storage_work_t g_async_work;
static struct k_work_sync g_async_sync;

static void async_write_handler(struct k_work *work)
{
    struct async_storage_work_t *w = CONTAINER_OF(work, struct async_storage_work_t, work_item);
    if (w->zfile) {
        w->res = fs_write(w->zfile, w->data, w->len);
    }
}
#endif

/**
 * Console byte read helper with Abort Key monitoring and timeout polling.
 */
static int console_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    if (g_active_channel && g_active_channel->read_byte) {
        return g_active_channel->read_byte(byte, timeout_ms, g_active_channel->user_data);
    }

    (void)user_data;
    uint8_t b = 0;
    bool got_byte = false;

    int64_t start = k_uptime_get();
    uint32_t wait_limit = (timeout_ms == 0) ? 1 : timeout_ms;

    while (k_uptime_get() - start <= wait_limit) {
#if defined(CONFIG_MODEM_RING_BUFFER)
        if (g_modem_settings.ring_buffer && !ring_buf_is_empty(&g_uart_ring_buf)) {
            if (ring_buf_get(&g_uart_ring_buf, &b, 1) == 1) {
                got_byte = true;
                break;
            }
        } else
#endif
        {
            int ch = console_getchar();
            if (ch >= 0) {
                b = (uint8_t)ch;
                got_byte = true;
                break;
            }
        }
        k_msleep(10);
    }

    if (!got_byte) {
        return -1;
    }

#if defined(CONFIG_MODEM_ABORT_KEY)
    if (g_modem_settings.abort_key) {
        if (b == g_modem_settings.abort_key_char) {
            return -2;   /* Transfer cancelled by user */
        }
        if (b == 0x18) { /* CAN */
            g_can_count++;
            if (g_can_count >= 2) {
                g_can_count = 0;
                return -2; /* Transfer cancelled by user */
            }
        } else {
            g_can_count = 0;
        }
    }
#endif

    *byte = b;
    return 0;
}

#if defined(CONFIG_MODEM_PROGRESS_BAR)
/**
 * Real-time shell progress bar renderer.
 */
static void update_progress_bar(console_modem_ctx_t *ctx)
{
    if (!g_modem_settings.progress_bar || !ctx || ctx->file_size == 0) return;
    int percent = (int)((ctx->bytes_transferred * 100) / ctx->file_size);
    if (percent > 100) percent = 100;

    int64_t elapsed_ms = k_uptime_get() - ctx->start_time_ms;
    double kb_s = (elapsed_ms > 0) ? ((double)ctx->bytes_transferred / 1024.0) / ((double)elapsed_ms / 1000.0) : 0.0;

    int bar_width = 20;
    int filled = (percent * bar_width) / 100;
    printf("\rProgress: [");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
    }
    printf("] %3d%% (%zu/%zu B, %.1f KB/s)", percent, ctx->bytes_transferred, ctx->file_size, kb_s);
    fflush(stdout);
}
#endif

/**
 * Standard I/O adapter writing bytes to Zephyr console UART.
 */
static int console_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    if (g_active_channel && g_active_channel->write_bytes) {
        return g_active_channel->write_bytes(buf, len, g_active_channel->user_data);
    }

    (void)user_data;
    for (size_t i = 0; i < len; i++) {
        console_putchar(buf[i]);
    }
    return 0;
}

#if defined(CONFIG_FILE_SYSTEM)

/**
 * Open file for writing on target storage system (Zephyr VFS).
 */
static int open_output_file(console_modem_ctx_t *ctx, const char *path)
{
    char full_path[256];
    if (g_modem_settings.default_target_dir[0] != '\0' && path[0] != '/') {
        snprintf(full_path, sizeof(full_path), "%s/%s", g_modem_settings.default_target_dir, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s", path);
    }

    /* File overwrite policy check */
    struct fs_dirent entry;
    if (fs_stat(full_path, &entry) == 0) {
        if (g_modem_settings.overwrite_mode == MODEM_OVERWRITE_SKIP) {
            return -2; /* Skip file */
        } else if (g_modem_settings.overwrite_mode == MODEM_OVERWRITE_ABORT) {
            return -3; /* Abort transfer */
        }
    }

    fs_file_t_init(&ctx->zfile);
    int res = fs_open(&ctx->zfile, full_path, FS_O_CREATE | FS_O_WRITE);
    if (res == 0) {
        ctx->zfile_open = true;
        ctx->start_time_ms = k_uptime_get();
        return 0;
    }
    return -1;
}

/**
 * Open file for reading from target storage system (Zephyr VFS).
 */
static int open_input_file(console_modem_ctx_t *ctx, const char *path)
{
    fs_file_t_init(&ctx->zfile);
    int res = fs_open(&ctx->zfile, path, FS_O_READ);
    if (res == 0) {
        ctx->zfile_open = true;
        struct fs_dirent entry;
        if (fs_stat(path, &entry) == 0) {
            ctx->file_size = entry.size;
        }
        return 0;
    }
    return -1;
}

/**
 * Write payload buffer chunk to active output file.
 */
static int write_file_data(console_modem_ctx_t *ctx, const uint8_t *buf, size_t len)
{
    if (!ctx->zfile_open) return -1;
    ssize_t res;

#if defined(CONFIG_MODEM_ASYNC_STORAGE)
    if (g_modem_settings.async_storage) {
        k_work_init(&g_async_work.work_item, async_write_handler);
        g_async_work.zfile = &ctx->zfile;
        size_t copy_len = (len > sizeof(g_async_work.data)) ? sizeof(g_async_work.data) : len;
        memcpy(g_async_work.data, buf, copy_len);
        g_async_work.len = copy_len;
        g_async_work.res = -1;

        k_work_submit(&g_async_work.work_item);
        k_work_flush(&g_async_work.work_item, &g_async_sync);
        res = g_async_work.res;
    } else
#endif
    {
        res = fs_write(&ctx->zfile, buf, len);
    }

    if (res < 0) return -1;
    ctx->bytes_transferred += (size_t)res;

#if defined(CONFIG_MODEM_PROGRESS_BAR)
    update_progress_bar(ctx);
#endif

    if (g_modem_settings.sync_interval_blocks > 0) {
        fs_sync(&ctx->zfile);
    }

    return ((size_t)res == len) ? 0 : -1;
}

/**
 * Read payload buffer chunk from active input file at offset.
 */
static int read_file_data(console_modem_ctx_t *ctx, size_t offset, uint8_t *buf, size_t len)
{
    if (!ctx->zfile_open) return -1;
    fs_seek(&ctx->zfile, offset, FS_SEEK_SET);
    ssize_t res = fs_read(&ctx->zfile, buf, len);
    return (res >= 0) ? (int)res : -1;
}

/**
 * Close active file handle on target file system.
 */
static void close_file(console_modem_ctx_t *ctx)
{
    if (ctx->zfile_open) {
        fs_close(&ctx->zfile);
        ctx->zfile_open = false;
    }
}

#if defined(CONFIG_MODEM_XMODEM)
/* XMODEM Data Callbacks */
static int xmodem_rx_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)block_num;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    return ctx ? write_file_data(ctx, buf, len) : -1;
}

static int xmodem_tx_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)block_num;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    size_t offset = ctx->bytes_transferred;
    int read_b = read_file_data(ctx, offset, (uint8_t *)buf, len);
    if (read_b > 0) {
        ctx->bytes_transferred += (size_t)read_b;
    }
    return (read_b >= 0) ? 0 : -1;
}
#endif

#if defined(CONFIG_MODEM_YMODEM)
/* YMODEM RX Callbacks */
static int ymodem_on_file_start(const ymodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;

    const char *fname = (info->filename[0] != '\0') ? info->filename : ctx->target_path;
    if (fname[0] == '\0') fname = "received_file.bin";

    if (open_output_file(ctx, fname) != 0) {
        return -1;
    }
    ctx->file_size = info->size;
    ctx->bytes_transferred = 0;
    return 0;
}

static int ymodem_on_file_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
    (void)offset;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    return ctx ? write_file_data(ctx, buf, len) : -1;
}

static void ymodem_on_file_end(const ymodem_file_info_t *info, ymodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (ctx) {
        close_file(ctx);
    }
}

/* YMODEM TX Callbacks */
static int ymodem_tx_get_file_info(size_t file_index, ymodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx || file_index > 0) return -1;

    const char *fname = strrchr(ctx->target_path, '/');
    fname = fname ? fname + 1 : ctx->target_path;
    strncpy(info->filename, fname, sizeof(info->filename) - 1);
    info->filename[sizeof(info->filename) - 1] = '\0';
    info->size = ctx->file_size;
    return 0;
}

static int ymodem_tx_read_file_data(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data)
{
    (void)file_index;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    return read_file_data(ctx, offset, buf, len);
}
#endif

#if defined(CONFIG_MODEM_ZMODEM)
/* ZMODEM RX Callbacks */
static int zmodem_on_file_start(const zmodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;

    const char *fname = (info->filename[0] != '\0') ? info->filename : ctx->target_path;
    if (fname[0] == '\0') fname = "received_file.bin";

    if (open_output_file(ctx, fname) != 0) {
        return -1;
    }
    ctx->file_size = info->size;
    ctx->bytes_transferred = 0;
    return 0;
}

static int zmodem_on_file_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
{
    (void)offset;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    return ctx ? write_file_data(ctx, buf, len) : -1;
}

static void zmodem_on_file_end(const zmodem_file_info_t *info, zmodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (ctx) {
        close_file(ctx);
    }
}

/* ZMODEM TX Callbacks */
static int zmodem_tx_get_file_metadata(size_t file_index, zmodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx || file_index > 0) return -1;

    const char *fname = strrchr(ctx->target_path, '/');
    fname = fname ? fname + 1 : ctx->target_path;
    strncpy(info->filename, fname, sizeof(info->filename) - 1);
    info->filename[sizeof(info->filename) - 1] = '\0';
    info->size = ctx->file_size;
    return 0;
}

static int zmodem_tx_read_file_bytes(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data)
{
    (void)file_index;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    return read_file_data(ctx, offset, buf, len);
}
#endif

/* High-level Console Receive Functions */
int console_modem_rx_xmodem(const char *output_filename)
{
#if defined(CONFIG_MODEM_XMODEM)
    console_modem_ctx_t ctx = {0};
    if (output_filename) {
        snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", output_filename);
        if (open_output_file(&ctx, output_filename) != 0) return -1;
    }

    xmodem_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .data_cb = xmodem_rx_data_cb,
        .user_data = &ctx
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.byte_timeout_ms = g_modem_settings.byte_timeout_ms;
    cfg.packet_timeout_ms = g_modem_settings.packet_timeout_ms;
    cfg.max_retries = g_modem_settings.max_retries;

    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, &cfg, &total_rx);

    close_file(&ctx);
    return (status == XMODEM_OK) ? 0 : -1;
#else
    (void)output_filename;
    return -1;
#endif
}

int console_modem_rx_ymodem(const char *output_filename)
{
#if defined(CONFIG_MODEM_YMODEM)
    console_modem_ctx_t ctx = {0};
    if (output_filename) {
        snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", output_filename);
    }

    ymodem_rx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .on_file_start = ymodem_on_file_start,
        .on_data = ymodem_on_file_data,
        .on_file_end = ymodem_on_file_end,
        .user_data = &ctx
    };

    ymodem_status_t status = ymodem_receive(&cbs);
    return (status == YMODEM_OK) ? 0 : -1;
#else
    (void)output_filename;
    return -1;
#endif
}

int console_modem_rx_zmodem(const char *output_filename)
{
#if defined(CONFIG_MODEM_ZMODEM)
    console_modem_ctx_t ctx = {0};
    if (output_filename) {
        snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", output_filename);
    }

    zmodem_rx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .on_file_start = zmodem_on_file_start,
        .on_data = zmodem_on_file_data,
        .on_file_end = zmodem_on_file_end,
        .user_data = &ctx
    };

    zmodem_status_t status = zmodem_receive(&cbs);
    return (status == ZMODEM_OK) ? 0 : -1;
#else
    (void)output_filename;
    return -1;
#endif
}

/* High-level Console Transmit Functions */
int console_modem_tx_xmodem(const char *input_filename)
{
#if defined(CONFIG_MODEM_XMODEM)
    if (!input_filename) return -1;
    console_modem_ctx_t ctx = {0};
    snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", input_filename);

    if (open_input_file(&ctx, input_filename) != 0) return -1;

    xmodem_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .data_cb = xmodem_tx_data_cb,
        .user_data = &ctx
    };

    xmodem_config_t cfg;
    xmodem_config_init(&cfg);
    cfg.byte_timeout_ms = g_modem_settings.byte_timeout_ms;
    cfg.packet_timeout_ms = g_modem_settings.packet_timeout_ms;
    cfg.max_retries = g_modem_settings.max_retries;

    xmodem_status_t status = xmodem_transmit(&cbs, ctx.file_size, &cfg);
    close_file(&ctx);
    return (status == XMODEM_OK) ? 0 : -1;
#else
    (void)input_filename;
    return -1;
#endif
}

int console_modem_tx_ymodem(const char *input_filename)
{
#if defined(CONFIG_MODEM_YMODEM)
    if (!input_filename) return -1;
    console_modem_ctx_t ctx = {0};
    snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", input_filename);

    if (open_input_file(&ctx, input_filename) != 0) return -1;

    ymodem_tx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .get_file_info = ymodem_tx_get_file_info,
        .read_data = ymodem_tx_read_file_data,
        .user_data = &ctx
    };

    ymodem_status_t status = ymodem_transmit(&cbs);
    close_file(&ctx);
    return (status == YMODEM_OK) ? 0 : -1;
#else
    (void)input_filename;
    return -1;
#endif
}

int console_modem_tx_zmodem(const char *input_filename)
{
#if defined(CONFIG_MODEM_ZMODEM)
    if (!input_filename) return -1;
    console_modem_ctx_t ctx = {0};
    snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", input_filename);

    if (open_input_file(&ctx, input_filename) != 0) return -1;

    zmodem_tx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .get_file_info = zmodem_tx_get_file_metadata,
        .read_data = zmodem_tx_read_file_bytes,
        .user_data = &ctx
    };

    zmodem_status_t status = zmodem_transmit(&cbs);
    close_file(&ctx);
    return (status == ZMODEM_OK) ? 0 : -1;
#else
    (void)input_filename;
    return -1;
#endif
}

int console_modem_tx_directory(const char *dir_path, int protocol)
{
#if defined(CONFIG_MODEM_DIRECTORY_TRANSFERS)
    if (!g_modem_settings.directory_transfers || !dir_path) return -1;

    struct fs_dir_t dir;
    fs_dir_t_init(&dir);
    int res = fs_opendir(&dir, dir_path);
    if (res != 0) return -1;

    struct fs_dirent entry;
    while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
        if (entry.type == FS_DIR_ENTRY_FILE) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry.name);

            if (protocol == 1) {
                console_modem_tx_ymodem(file_path);
            } else {
                console_modem_tx_zmodem(file_path);
            }
        }
    }
    fs_closedir(&dir);
    return 0;
#else
    (void)dir_path;
    (void)protocol;
    return -1;
#endif
}

int console_modem_mcuboot_update(const char *output_filename, int protocol)
{
#if defined(CONFIG_MODEM_MCUBOOT_UPDATE)
    if (!g_modem_settings.mcuboot_update) return -1;
    const char *target = (output_filename && output_filename[0] != '\0') ? output_filename : "firmware.bin";

    if (protocol == 1) {
        return console_modem_rx_ymodem(target);
    } else if (protocol == 2) {
        return console_modem_rx_xmodem(target);
    } else {
        return console_modem_rx_zmodem(target);
    }
#else
    (void)output_filename;
    (void)protocol;
    return -1;
#endif
}

#endif /* CONFIG_FILE_SYSTEM */

/* Zephyr Shell Commands Registration */
#if defined(CONFIG_SHELL)

#if defined(CONFIG_FILE_SYSTEM)
static int cmd_modem_rx(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    const char *proto = "zmodem";
    const char *out_path = NULL;

    if (argc > 1) {
        proto = argv[1];
    }
    if (argc > 2) {
        out_path = argv[2];
    }

#if defined(CONFIG_MODEM_FLASH_PARTITION)
    if (out_path && strncmp(out_path, "flash:", 6) == 0) {
        strncpy(g_modem_settings.flash_partition, out_path + 6, sizeof(g_modem_settings.flash_partition) - 1);
        out_path = "flash_image.bin";
    }
#endif

#if defined(CONFIG_MODEM_STATS)
    modem_stats_t stats;
    console_modem_stats_get(&stats);
    stats.total_transfers++;
#endif

    int res = 0;
    if (proto && (strcmp(proto, "xmodem") == 0 || strcmp(proto, "x") == 0)) {
        res = console_modem_rx_xmodem(out_path ? out_path : "xmodem.bin");
    } else if (proto && (strcmp(proto, "ymodem") == 0 || strcmp(proto, "y") == 0)) {
        res = console_modem_rx_ymodem(out_path);
    } else if (proto && (strcmp(proto, "zmodem") == 0 || strcmp(proto, "z") == 0)) {
        res = console_modem_rx_zmodem(out_path);
    } else {
        res = console_modem_rx_zmodem(argv[1]);
    }

    return res;
}

static int cmd_modem_tx(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    if (argc < 2) return -1;

    const char *proto = argv[1];
    const char *in_path = (argc > 2) ? argv[2] : NULL;

    if (argc == 2) {
        in_path = argv[1];
        proto = "zmodem";
    }

    struct fs_dirent entry;
    if (in_path && fs_stat(in_path, &entry) == 0) {
        if (entry.type == FS_DIR_ENTRY_DIR) {
            int p_code = (proto && (strcmp(proto, "ymodem") == 0 || strcmp(proto, "y") == 0)) ? 1 : 2;
            return console_modem_tx_directory(in_path, p_code);
        }
    }

    if (proto && (strcmp(proto, "xmodem") == 0 || strcmp(proto, "x") == 0)) {
        return console_modem_tx_xmodem(in_path);
    } else if (proto && (strcmp(proto, "ymodem") == 0 || strcmp(proto, "y") == 0)) {
        return console_modem_tx_ymodem(in_path);
    } else {
        return console_modem_tx_zmodem(in_path);
    }
}
#endif /* CONFIG_FILE_SYSTEM */

#if defined(CONFIG_MODEM_STATS)
static int cmd_modem_stats(const struct shell *sh, size_t argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "reset") == 0) {
        console_modem_stats_reset();
        shell_print(sh, "Modem statistics reset.");
        return 0;
    }

    modem_stats_t stats;
    console_modem_stats_get(&stats);

    shell_print(sh, "Modem Transfer Statistics:");
    shell_print(sh, "  Total Transfers:       %u", stats.total_transfers);
    shell_print(sh, "  Successful Transfers:  %u", stats.successful_transfers);
    shell_print(sh, "  Failed Transfers:      %u", stats.failed_transfers);
    shell_print(sh, "  CRC Errors:            %u", stats.crc_errors);
    shell_print(sh, "  Packet Retries:        %u", stats.retries);
    shell_print(sh, "  Total Bytes Received:  %zu", stats.total_bytes_rx);
    shell_print(sh, "  Total Bytes Sent:      %zu", stats.total_bytes_tx);
    return 0;
}
#endif

static int cmd_modem_config(const struct shell *sh, size_t argc, char **argv)
{
    if (argc == 1) {
        shell_print(sh, "Modem Configuration:");
        shell_print(sh, "  Packet Timeout:      %u ms", g_modem_settings.packet_timeout_ms);
        shell_print(sh, "  Byte Timeout:        %u ms", g_modem_settings.byte_timeout_ms);
        shell_print(sh, "  Max Retries:         %u", g_modem_settings.max_retries);
        shell_print(sh, "  Inter-block Delay:   %u ms", g_modem_settings.inter_block_delay_ms);
        shell_print(sh, "  Handshake Delay:     %u ms", g_modem_settings.handshake_delay_ms);
        shell_print(sh, "  Overwrite Mode:      %d", (int)g_modem_settings.overwrite_mode);
#if defined(CONFIG_MODEM_ENABLE_RESUME)
        shell_print(sh, "  Auto-Resume:         %s", g_modem_settings.enable_resume ? "true" : "false");
#endif
        shell_print(sh, "  Target Directory:    %s", g_modem_settings.default_target_dir[0] ? g_modem_settings.default_target_dir : "(root)");
        shell_print(sh, "  Sync Interval:       %u blocks", g_modem_settings.sync_interval_blocks);
#if defined(CONFIG_MODEM_AUTO_START)
        shell_print(sh, "  Auto-Start:          %s", g_modem_settings.auto_start ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ASYNC_STORAGE)
        shell_print(sh, "  Async Storage:       %s", g_modem_settings.async_storage ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_PROGRESS_BAR)
        shell_print(sh, "  Progress Bar:        %s", g_modem_settings.progress_bar ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_DIRECTORY_TRANSFERS)
        shell_print(sh, "  Directory Transfers: %s", g_modem_settings.directory_transfers ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_RING_BUFFER)
        shell_print(sh, "  Ring Buffer Transport: %s", g_modem_settings.ring_buffer ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ABORT_KEY)
        shell_print(sh, "  Abort Key Monitor:   %s", g_modem_settings.abort_key ? "true" : "false");
        shell_print(sh, "  Abort Key Char:      0x%02X (%u)", g_modem_settings.abort_key_char, g_modem_settings.abort_key_char);
#endif
#if defined(CONFIG_MODEM_FLOW_CONTROL)
        shell_print(sh, "  Flow Control:        %s", g_modem_settings.flow_control ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_FLASH_PARTITION)
        shell_print(sh, "  Flash Partition:     %s", g_modem_settings.flash_partition[0] ? g_modem_settings.flash_partition : "(none)");
#endif
#if defined(CONFIG_MODEM_MCUBOOT_UPDATE)
        shell_print(sh, "  MCUBoot Update:      %s", g_modem_settings.mcuboot_update ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_NVS_CHECKPOINTS)
        shell_print(sh, "  NVS Checkpoints:     %s", g_modem_settings.nvs_checkpoints ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_CRYPTO_STREAM)
        shell_print(sh, "  Crypto Stream:       %s", g_modem_settings.crypto_stream ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_UART_DMA)
        shell_print(sh, "  UART DMA Adapter:    %s", g_modem_settings.uart_dma ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_USB_CDC_ACM)
        shell_print(sh, "  USB CDC-ACM Adapter: %s", g_modem_settings.usb_cdc_acm ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_MCUBOOT_VALIDATE)
        shell_print(sh, "  MCUBoot Validate:    %s", g_modem_settings.mcuboot_validate ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_SIGNATURE_VERIFY)
        shell_print(sh, "  Signature Verify:    %s", g_modem_settings.signature_verify ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ENCRYPTED_STREAM)
        shell_print(sh, "  Encrypted Envelope:  %s", g_modem_settings.encrypted_envelope ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_SESSION_DISPATCHER)
        shell_print(sh, "  Session Dispatcher:  %s", g_modem_settings.session_dispatcher ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_LOG_ROTATION)
        shell_print(sh, "  Log Rotation:        %s", g_modem_settings.log_rotation ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_NFC)
        shell_print(sh, "  NFC Transport:       %s", g_modem_settings.nfc_transport ? "true" : "false");
#endif
        return 0;
    }

    if (argc >= 3) {
        const char *param = argv[1];
        const char *val_str = argv[2];
        uint32_t val = (uint32_t)strtoul(val_str, NULL, 10);

        if (strcmp(param, "packet_timeout") == 0 || strcmp(param, "pkt_timeout") == 0) {
            g_modem_settings.packet_timeout_ms = val;
            shell_print(sh, "Packet timeout set to %u ms", val);
        } else if (strcmp(param, "byte_timeout") == 0) {
            g_modem_settings.byte_timeout_ms = val;
            shell_print(sh, "Byte timeout set to %u ms", val);
        } else if (strcmp(param, "max_retries") == 0 || strcmp(param, "retries") == 0) {
            g_modem_settings.max_retries = (uint8_t)val;
            shell_print(sh, "Max retries set to %u", val);
        } else if (strcmp(param, "inter_block_delay") == 0) {
            g_modem_settings.inter_block_delay_ms = val;
            shell_print(sh, "Inter-block delay set to %u ms", val);
        } else if (strcmp(param, "handshake_delay") == 0) {
            g_modem_settings.handshake_delay_ms = val;
            shell_print(sh, "Handshake delay set to %u ms", val);
        } else if (strcmp(param, "overwrite_mode") == 0) {
            g_modem_settings.overwrite_mode = (modem_overwrite_mode_t)val;
            shell_print(sh, "Overwrite mode set to %d", (int)val);
#if defined(CONFIG_MODEM_ENABLE_RESUME)
        } else if (strcmp(param, "enable_resume") == 0 || strcmp(param, "resume") == 0) {
            g_modem_settings.enable_resume = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Auto-resume set to %s", g_modem_settings.enable_resume ? "true" : "false");
#endif
        } else if (strcmp(param, "target_dir") == 0) {
            strncpy(g_modem_settings.default_target_dir, val_str, sizeof(g_modem_settings.default_target_dir) - 1);
            g_modem_settings.default_target_dir[sizeof(g_modem_settings.default_target_dir) - 1] = '\0';
            shell_print(sh, "Default target directory set to %s", g_modem_settings.default_target_dir);
        } else if (strcmp(param, "sync_interval") == 0) {
            g_modem_settings.sync_interval_blocks = val;
            shell_print(sh, "Sync interval set to %u blocks", val);
#if defined(CONFIG_MODEM_AUTO_START)
        } else if (strcmp(param, "auto_start") == 0) {
            g_modem_settings.auto_start = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Auto-start set to %s", g_modem_settings.auto_start ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ASYNC_STORAGE)
        } else if (strcmp(param, "async_storage") == 0) {
            g_modem_settings.async_storage = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Async storage set to %s", g_modem_settings.async_storage ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_PROGRESS_BAR)
        } else if (strcmp(param, "progress_bar") == 0) {
            g_modem_settings.progress_bar = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Progress bar set to %s", g_modem_settings.progress_bar ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_DIRECTORY_TRANSFERS)
        } else if (strcmp(param, "directory_transfers") == 0) {
            g_modem_settings.directory_transfers = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Directory transfers set to %s", g_modem_settings.directory_transfers ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_RING_BUFFER)
        } else if (strcmp(param, "ring_buffer") == 0) {
            g_modem_settings.ring_buffer = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Ring buffer transport set to %s", g_modem_settings.ring_buffer ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ABORT_KEY)
        } else if (strcmp(param, "abort_key") == 0) {
            g_modem_settings.abort_key = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Abort key monitor set to %s", g_modem_settings.abort_key ? "true" : "false");
        } else if (strcmp(param, "abort_char") == 0 || strcmp(param, "abort_key_char") == 0) {
            g_modem_settings.abort_key_char = (uint8_t)val;
            shell_print(sh, "Abort key character set to 0x%02X (%u)", g_modem_settings.abort_key_char, g_modem_settings.abort_key_char);
#endif
#if defined(CONFIG_MODEM_FLOW_CONTROL)
        } else if (strcmp(param, "flow_control") == 0) {
            g_modem_settings.flow_control = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Flow control set to %s", g_modem_settings.flow_control ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_FLASH_PARTITION)
        } else if (strcmp(param, "flash_partition") == 0 || strcmp(param, "partition") == 0) {
            strncpy(g_modem_settings.flash_partition, val_str, sizeof(g_modem_settings.flash_partition) - 1);
            g_modem_settings.flash_partition[sizeof(g_modem_settings.flash_partition) - 1] = '\0';
            shell_print(sh, "Target flash partition set to %s", g_modem_settings.flash_partition);
#endif
#if defined(CONFIG_MODEM_MCUBOOT_UPDATE)
        } else if (strcmp(param, "mcuboot_update") == 0) {
            g_modem_settings.mcuboot_update = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "MCUBoot update set to %s", g_modem_settings.mcuboot_update ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_NVS_CHECKPOINTS)
        } else if (strcmp(param, "nvs_checkpoints") == 0) {
            g_modem_settings.nvs_checkpoints = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "NVS checkpoints set to %s", g_modem_settings.nvs_checkpoints ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_CRYPTO_STREAM)
        } else if (strcmp(param, "crypto_stream") == 0) {
            g_modem_settings.crypto_stream = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Crypto stream set to %s", g_modem_settings.crypto_stream ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_UART_DMA)
        } else if (strcmp(param, "uart_dma") == 0) {
            g_modem_settings.uart_dma = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "UART DMA adapter set to %s", g_modem_settings.uart_dma ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_USB_CDC_ACM)
        } else if (strcmp(param, "usb_cdc_acm") == 0) {
            g_modem_settings.usb_cdc_acm = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "USB CDC-ACM adapter set to %s", g_modem_settings.usb_cdc_acm ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_MCUBOOT_VALIDATE)
        } else if (strcmp(param, "mcuboot_validate") == 0) {
            g_modem_settings.mcuboot_validate = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "MCUBoot validation set to %s", g_modem_settings.mcuboot_validate ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_SIGNATURE_VERIFY)
        } else if (strcmp(param, "signature_verify") == 0) {
            g_modem_settings.signature_verify = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Signature verify set to %s", g_modem_settings.signature_verify ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_ENCRYPTED_STREAM)
        } else if (strcmp(param, "encrypted_envelope") == 0) {
            g_modem_settings.encrypted_envelope = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Encrypted envelope set to %s", g_modem_settings.encrypted_envelope ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_SESSION_DISPATCHER)
        } else if (strcmp(param, "session_dispatcher") == 0) {
            g_modem_settings.session_dispatcher = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Session dispatcher set to %s", g_modem_settings.session_dispatcher ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_LOG_ROTATION)
        } else if (strcmp(param, "log_rotation") == 0) {
            g_modem_settings.log_rotation = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "Log rotation set to %s", g_modem_settings.log_rotation ? "true" : "false");
#endif
#if defined(CONFIG_MODEM_NFC)
        } else if (strcmp(param, "nfc_transport") == 0 || strcmp(param, "nfc") == 0) {
            g_modem_settings.nfc_transport = (strcmp(val_str, "true") == 0 || strcmp(val_str, "1") == 0);
            shell_print(sh, "NFC transport set to %s", g_modem_settings.nfc_transport ? "true" : "false");
#endif
        } else {
            shell_error(sh, "Unknown configuration parameter: %s", param);
            return -1;
        }
        return 0;
    }

    shell_error(sh, "Usage: modem config [param val]");
    return -1;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
#if defined(CONFIG_FILE_SYSTEM)
    SHELL_CMD_ARG(rx, NULL, "Receive file: modem rx [x|y|z] [file]", cmd_modem_rx, 1, 2),
    SHELL_CMD_ARG(tx, NULL, "Transmit file: modem tx [x|y|z] <file>", cmd_modem_tx, 1, 2),
    SHELL_CMD_ARG(update, NULL, "Stream MCUBoot update: modem update [x|y|z] <file>", cmd_modem_rx, 1, 2),
#endif
    SHELL_CMD_ARG(config, NULL, "Configure modem settings: modem config [param val]", cmd_modem_config, 1, 2),
#if defined(CONFIG_MODEM_STATS)
    SHELL_CMD_ARG(stats, NULL, "Display transfer stats: modem stats [reset]", cmd_modem_stats, 1, 1),
#endif
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &sub_modem, "Serial transfer protocols (XMODEM/YMODEM/ZMODEM)", NULL);

#if defined(CONFIG_FILE_SYSTEM)
SHELL_CMD_REGISTER(mrx, NULL, "Short command for modem rx: mrx [x|y|z] [file]", cmd_modem_rx);
SHELL_CMD_REGISTER(mtx, NULL, "Short command for modem tx: mtx [x|y|z] <file>", cmd_modem_tx);
#endif

#endif /* CONFIG_SHELL */

int zephyr_console_modem_init(void)
{
    return 0;
}
