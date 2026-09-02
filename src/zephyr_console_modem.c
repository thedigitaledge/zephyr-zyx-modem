/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Console / Shell Serial Modem Protocol Adapter.
 * Integrates XMODEM, YMODEM, and ZMODEM file transfer routines
 * with Zephyr shell commands and Zephyr VFS file systems.
 */

#include "modem/zephyr_console_modem.h"
#include "modem/xmodem.h"
#include "modem/ymodem.h"
#include "modem/zmodem.h"

#include <stdio.h>
#include <string.h>

#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/console/console.h>
#if defined(CONFIG_FILE_SYSTEM)
#include <zephyr/fs/fs.h>
#endif
#endif

/**
 * Context structure managing current console transfer state and file handle.
 */
typedef struct {
    const void *shell_ctx;
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
    struct fs_file_t zfile;
    bool zfile_open;
#else
    FILE *file_handle;
#endif
    size_t file_size;
    size_t bytes_transferred;
    char target_path[256];
} console_modem_ctx_t;

/**
 * Standard I/O adapter reading a single character from Zephyr console UART.
 */
static int console_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
#if defined(__ZEPHYR__)
    int ch = console_getchar();
    if (ch < 0) {
        return -1;
    }
    *byte = (uint8_t)ch;
    return 0;
#else
    int c = getchar();
    if (c == EOF) {
        return -1;
    }
    *byte = (uint8_t)c;
    return 0;
#endif
}

/**
 * Standard I/O adapter writing bytes to Zephyr console UART.
 */
static int console_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
#if defined(__ZEPHYR__)
    for (size_t i = 0; i < len; i++) {
        console_putchar(buf[i]);
    }
    return 0;
#else
    size_t written = fwrite(buf, 1, len, stdout);
    (void)written;
    fflush(stdout);
    return 0;
#endif
}

/**
 * Open file for writing on target storage system (Zephyr VFS or standard C fopen).
 */
static int open_output_file(console_modem_ctx_t *ctx, const char *path)
{
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
    fs_file_t_init(&ctx->zfile);
    int res = fs_open(&ctx->zfile, path, FS_O_CREATE | FS_O_WRITE);
    if (res == 0) {
        ctx->zfile_open = true;
        return 0;
    }
    return -1;
#else
    ctx->file_handle = fopen(path, "wb");
    return (ctx->file_handle != NULL) ? 0 : -1;
#endif
}

/**
 * Open file for reading from target storage system.
 */
static int open_input_file(console_modem_ctx_t *ctx, const char *path)
{
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
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
#else
    ctx->file_handle = fopen(path, "rb");
    if (ctx->file_handle) {
        fseek(ctx->file_handle, 0, SEEK_END);
        ctx->file_size = (size_t)ftell(ctx->file_handle);
        fseek(ctx->file_handle, 0, SEEK_SET);
        return 0;
    }
    return -1;
#endif
}

/**
 * Write payload buffer chunk to active output file.
 */
static int write_output_file(console_modem_ctx_t *ctx, const uint8_t *buf, size_size_t len)
{
    (void)len;
    return -1;
}

static int write_file_data(console_modem_ctx_t *ctx, const uint8_t *buf, size_t len)
{
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
    if (!ctx->zfile_open) return -1;
    ssize_t res = fs_write(&ctx->zfile, buf, len);
    if (res < 0) return -1;
    ctx->bytes_transferred += (size_t)res;
    return ((size_t)res == len) ? 0 : -1;
#else
    if (!ctx->file_handle) return -1;
    size_t written = fwrite(buf, 1, len, ctx->file_handle);
    ctx->bytes_transferred += written;
    return (written == len) ? 0 : -1;
#endif
}

/**
 * Read payload buffer chunk from active input file at offset.
 */
static int read_input_file(console_modem_ctx_t *ctx, size_t offset, uint8_t *buf, size_t len)
{
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
    if (!ctx->zfile_open) return -1;
    fs_seek(&ctx->zfile, offset, FS_SEEK_SET);
    ssize_t res = fs_read(&ctx->zfile, buf, len);
    return (res >= 0) ? (int)res : -1;
#else
    if (!ctx->file_handle) return -1;
    fseek(ctx->file_handle, (long)offset, SEEK_SET);
    size_t read_bytes = fread(buf, 1, len, ctx->file_handle);
    return (int)read_bytes;
#endif
}

/**
 * Close active file handle on target file system.
 */
static void close_file(console_modem_ctx_t *ctx)
{
#if defined(__ZEPHYR__) && defined(CONFIG_FILE_SYSTEM)
    if (ctx->zfile_open) {
        fs_close(&ctx->zfile);
        ctx->zfile_open = false;
    }
#else
    if (ctx->file_handle) {
        fclose(ctx->file_handle);
        ctx->file_handle = NULL;
    }
#endif
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
    int read_b = read_input_file(ctx, offset, buf, len);
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

static int ymodem_tx_read_data(size_t file_index, size_t offset, uint8_t *buf, size_t len, void *user_data)
{
    (void)file_index;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;
    return read_input_file(ctx, offset, buf, len);
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

static int zmodem_on_data(const uint8_t *buf, size_t len, size_t offset, void *user_data)
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
    return read_input_file(ctx, offset, buf, len);
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

    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, NULL, &total_rx);

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
        .on_data = zmodem_on_data,
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

    xmodem_status_t status = xmodem_transmit(&cbs, ctx.file_size, NULL);
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
        .read_data = ymodem_tx_read_data,
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

/* Zephyr Shell Commands Registration */
#if defined(__ZEPHYR__) && defined(CONFIG_SHELL)

static int cmd_modem_rx(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    const char *out_path = (argc > 1) ? argv[1] : "xmodem.bin";
    return console_modem_rx_xmodem(out_path);
}

static int cmd_modem_ry(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    const char *out_path = (argc > 1) ? argv[1] : NULL;
    return console_modem_rx_ymodem(out_path);
}

static int cmd_modem_rz(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    const char *out_path = (argc > 1) ? argv[1] : NULL;
    return console_modem_rx_zmodem(out_path);
}

static int cmd_modem_sx(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    if (argc < 2) return -1;
    return console_modem_tx_xmodem(argv[1]);
}

static int cmd_modem_sy(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    if (argc < 2) return -1;
    return console_modem_tx_ymodem(argv[1]);
}

static int cmd_modem_sz(const struct shell *sh, size_t argc, char **argv)
{
    (void)sh;
    if (argc < 2) return -1;
    return console_modem_tx_zmodem(argv[1]);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
    SHELL_CMD_ARG(rx, NULL, "Receive file using XMODEM: modem rx <file>", cmd_modem_rx, 1, 1),
    SHELL_CMD_ARG(ry, NULL, "Receive file using YMODEM: modem ry [file]", cmd_modem_ry, 1, 1),
    SHELL_CMD_ARG(rz, NULL, "Receive file using ZMODEM: modem rz [file]", cmd_modem_rz, 1, 1),
    SHELL_CMD_ARG(sx, NULL, "Send file using XMODEM: modem sx <file>", cmd_modem_sx, 2, 0),
    SHELL_CMD_ARG(sy, NULL, "Send file using YMODEM: modem sy <file>", cmd_modem_sy, 2, 0),
    SHELL_CMD_ARG(sz, NULL, "Send file using ZMODEM: modem sz <file>", cmd_modem_sz, 2, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &sub_modem, "Serial transfer protocols (XMODEM/YMODEM/ZMODEM)", NULL);
SHELL_CMD_REGISTER(rx, NULL, "Alias for modem rx (XMODEM receive)", cmd_modem_rx);
SHELL_CMD_REGISTER(ry, NULL, "Alias for modem ry (YMODEM receive)", cmd_modem_ry);
SHELL_CMD_REGISTER(rz, NULL, "Alias for modem rz (ZMODEM receive)", cmd_modem_rz);
SHELL_CMD_REGISTER(sx, NULL, "Alias for modem sx (XMODEM send)", cmd_modem_sx);
SHELL_CMD_REGISTER(sy, NULL, "Alias for modem sy (YMODEM send)", cmd_modem_sy);
SHELL_CMD_REGISTER(sz, NULL, "Alias for modem sz (ZMODEM send)", cmd_modem_sz);

#endif

int zephyr_console_modem_init(void)
{
    return 0;
}
