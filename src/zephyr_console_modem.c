/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Console / Shell Serial Modem Protocol Adapter.
 * Integrates XMODEM, YMODEM, and ZMODEM file transfer routines
 * with Zephyr shell commands and Zephyr VFS file systems.
 */

#include "zephyr_console_modem.h"
#include "modem/xmodem.h"
#include "modem/ymodem.h"
#include "modem/zmodem.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
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

/* Runtime Modem Configuration */
static console_modem_settings_t g_modem_settings = {
    .packet_timeout_ms = CONFIG_MODEM_PACKET_TIMEOUT_MS,
    .byte_timeout_ms = CONFIG_MODEM_BYTE_TIMEOUT_MS,
    .max_retries = CONFIG_MODEM_MAX_RETRIES
};

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
    char target_path[256];
} console_modem_ctx_t;

/**
 * Console byte read helper.
 */
static int console_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
    int ch = console_getchar();
    if (ch < 0) {
        return -1;
    }
    *byte = (uint8_t)ch;
    return 0;
}

/**
 * Standard I/O adapter writing bytes to Zephyr console UART.
 */
static int console_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
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
    fs_file_t_init(&ctx->zfile);
    int res = fs_open(&ctx->zfile, path, FS_O_CREATE | FS_O_WRITE);
    if (res == 0) {
        ctx->zfile_open = true;
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
    ssize_t res = fs_write(&ctx->zfile, buf, len);
    if (res < 0) return -1;
    ctx->bytes_transferred += (size_t)res;
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

/* XMODEM Data Callbacks */
static int xmodem_rx_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)block_num;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    return ctx ? write_file_data(ctx, buf, len) : -1;
}

static int xmodem_tx_data_cb(uint32_t block_num, uint8_t *buf, size_t len, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    size_t offset = (block_num - 1) * len;
    int read_b = read_file_data(ctx, offset, buf, len);
    return (read_b >= 0) ? 0 : -1;
}

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

static int ymodem_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
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
static int zmodem_tx_get_file_info(size_t file_index, zmodem_file_info_t *info, void *user_data)
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

static int zmodem_tx_read_data(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data)
{
    (void)file_index;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    return read_file_data(ctx, offset, buf, len);
}

/* High-level Console Receive Functions */
int console_modem_rx_xmodem(const char *output_filename)
{
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
}

int console_modem_rx_ymodem(const char *output_filename)
{
    console_modem_ctx_t ctx = {0};
    if (output_filename) {
        snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", output_filename);
    }

    ymodem_rx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .on_file_start = ymodem_on_file_start,
        .on_data = ymodem_on_data,
        .on_file_end = ymodem_on_file_end,
        .user_data = &ctx
    };

    ymodem_status_t status = ymodem_receive(&cbs);
    return (status == YMODEM_OK) ? 0 : -1;
}

int console_modem_rx_zmodem(const char *output_filename)
{
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
}

/* High-level Console Transmit Functions */
int console_modem_tx_xmodem(const char *input_filename)
{
    if (!input_filename) return -1;
    console_modem_ctx_t ctx = {0};
    snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", input_filename);

    if (open_input_file(&ctx, input_filename) != 0) return -1;

    xmodem_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .data_cb = (int (*)(uint32_t, const uint8_t *, size_t, void *))xmodem_tx_data_cb,
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
}

int console_modem_tx_ymodem(const char *input_filename)
{
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
}

int console_modem_tx_zmodem(const char *input_filename)
{
    if (!input_filename) return -1;
    console_modem_ctx_t ctx = {0};
    snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", input_filename);

    if (open_input_file(&ctx, input_filename) != 0) return -1;

    zmodem_tx_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .get_file_info = zmodem_tx_get_file_info,
        .read_data = zmodem_tx_read_data,
        .user_data = &ctx
    };

    zmodem_status_t status = zmodem_transmit(&cbs);
    close_file(&ctx);
    return (status == ZMODEM_OK) ? 0 : -1;
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

    if (proto && (strcmp(proto, "xmodem") == 0 || strcmp(proto, "x") == 0)) {
        return console_modem_rx_xmodem(out_path ? out_path : "xmodem.bin");
    } else if (proto && (strcmp(proto, "ymodem") == 0 || strcmp(proto, "y") == 0)) {
        return console_modem_rx_ymodem(out_path);
    } else if (proto && (strcmp(proto, "zmodem") == 0 || strcmp(proto, "z") == 0)) {
        return console_modem_rx_zmodem(out_path);
    } else {
        return console_modem_rx_zmodem(argv[1]);
    }
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

    if (proto && (strcmp(proto, "xmodem") == 0 || strcmp(proto, "x") == 0)) {
        return console_modem_tx_xmodem(in_path);
    } else if (proto && (strcmp(proto, "ymodem") == 0 || strcmp(proto, "y") == 0)) {
        return console_modem_tx_ymodem(in_path);
    } else {
        return console_modem_tx_zmodem(in_path);
    }
}
#endif /* CONFIG_FILE_SYSTEM */

static int cmd_modem_config(const struct shell *sh, size_t argc, char **argv)
{
    if (argc == 1) {
        shell_print(sh, "Modem Configuration:");
        shell_print(sh, "  Packet Timeout: %u ms", g_modem_settings.packet_timeout_ms);
        shell_print(sh, "  Byte Timeout:   %u ms", g_modem_settings.byte_timeout_ms);
        shell_print(sh, "  Max Retries:    %u", g_modem_settings.max_retries);
        return 0;
    }

    if (argc >= 3) {
        const char *param = argv[1];
        uint32_t val = (uint32_t)strtoul(argv[2], NULL, 10);

        if (strcmp(param, "packet_timeout") == 0 || strcmp(param, "pkt_timeout") == 0) {
            g_modem_settings.packet_timeout_ms = val;
            shell_print(sh, "Packet timeout set to %u ms", val);
        } else if (strcmp(param, "byte_timeout") == 0) {
            g_modem_settings.byte_timeout_ms = val;
            shell_print(sh, "Byte timeout set to %u ms", val);
        } else if (strcmp(param, "max_retries") == 0 || strcmp(param, "retries") == 0) {
            g_modem_settings.max_retries = (uint8_t)val;
            shell_print(sh, "Max retries set to %u", val);
        } else {
            shell_error(sh, "Unknown configuration parameter: %s", param);
            return -1;
        }
        return 0;
    }

    shell_error(sh, "Usage: modem config [packet_timeout|byte_timeout|max_retries <val>]");
    return -1;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
#if defined(CONFIG_FILE_SYSTEM)
    SHELL_CMD_ARG(rx, NULL, "Receive file: modem rx [x|y|z] [file]", cmd_modem_rx, 1, 2),
    SHELL_CMD_ARG(tx, NULL, "Transmit file: modem tx [x|y|z] <file>", cmd_modem_tx, 2, 1),
#endif
    SHELL_CMD_ARG(config, NULL, "Configure modem timeouts/retries: modem config [param val]", cmd_modem_config, 1, 2),
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
