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

/* Context for console shell transfer */
typedef struct {
    const void *shell_ctx;
    FILE *file_handle;
    size_t file_size;
    size_t bytes_transferred;
    char target_path[256];
} console_modem_ctx_t;

/* Standard read/write adapters for Zephyr console / standard C I/O */
static int console_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
#if defined(__ZEPHYR__)
    int ch = console_getchar();
    if (ch < 0) {
        return -1;
    }
    *byte = (uint8_t)ch;
    return 0;
#else
    (void)timeout_ms;
    (void)user_data;
    int c = getchar();
    if (c == EOF) {
        return -1;
    }
    *byte = (uint8_t)c;
    return 0;
#endif
}

static int console_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
#if defined(__ZEPHYR__)
    for (size_t i = 0; i < len; i++) {
        console_putchar(buf[i]);
    }
    return 0;
#else
    (void)user_data;
    size_t written = fwrite(buf, 1, len, stdout);
    fflush(stdout);
    return (written == len) ? 0 : -1;
#endif
}

/* XMODEM Data Callback */
static int xmodem_data_cb(uint32_t block_num, const uint8_t *buf, size_t len, void *user_data)
{
    (void)block_num;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (ctx && ctx->file_handle) {
        size_t written = fwrite(buf, 1, len, ctx->file_handle);
        ctx->bytes_transferred += written;
        return (written == len) ? 0 : -1;
    }
    return 0;
}

/* YMODEM RX Callbacks */
static int ymodem_on_file_start(const ymodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;

    const char *fname = (info->filename[0] != '\0') ? info->filename : ctx->target_path;
    if (fname[0] == '\0') fname = "received_file.bin";

    ctx->file_handle = fopen(fname, "wb");
    if (!ctx->file_handle) {
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
    if (ctx && ctx->file_handle) {
        size_t written = fwrite(buf, 1, len, ctx->file_handle);
        ctx->bytes_transferred += written;
        return (written == len) ? 0 : -1;
    }
    return -1;
}

static void ymodem_on_file_end(const ymodem_file_info_t *info, ymodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (ctx && ctx->file_handle) {
        fclose(ctx->file_handle);
        ctx->file_handle = NULL;
    }
}

/* ZMODEM RX Callbacks */
static int zmodem_on_file_start(const zmodem_file_info_t *info, void *user_data)
{
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (!ctx) return -1;

    const char *fname = (info->filename[0] != '\0') ? info->filename : ctx->target_path;
    if (fname[0] == '\0') fname = "received_file.bin";

    ctx->file_handle = fopen(fname, "wb");
    if (!ctx->file_handle) {
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
    if (ctx && ctx->file_handle) {
        size_t written = fwrite(buf, 1, len, ctx->file_handle);
        ctx->bytes_transferred += written;
        return (written == len) ? 0 : -1;
    }
    return -1;
}

static void zmodem_on_file_end(const zmodem_file_info_t *info, zmodem_status_t status, void *user_data)
{
    (void)info;
    (void)status;
    console_modem_ctx_t *ctx = (console_modem_ctx_t *)user_data;
    if (ctx && ctx->file_handle) {
        fclose(ctx->file_handle);
        ctx->file_handle = NULL;
    }
}

/* High-level Console Receive Functions */
int console_modem_rx_xmodem(const char *output_filename)
{
    console_modem_ctx_t ctx = {0};
    if (output_filename) {
        snprintf(ctx.target_path, sizeof(ctx.target_path), "%s", output_filename);
        ctx.file_handle = fopen(output_filename, "wb");
        if (!ctx.file_handle) return -1;
    }

    xmodem_callbacks_t cbs = {
        .read_byte = console_read_byte,
        .write_bytes = console_write_bytes,
        .data_cb = xmodem_data_cb,
        .user_data = &ctx
    };

    size_t total_rx = 0;
    xmodem_status_t status = xmodem_receive(&cbs, NULL, &total_rx);

    if (ctx.file_handle) {
        fclose(ctx.file_handle);
    }

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

/* Zephyr Shell Commands Handlers */
#if defined(__ZEPHYR__) && defined(CONFIG_SHELL)

static int cmd_modem_rx(const struct shell *sh, size_t argc, char **argv)
{
    const char *out_path = (argc > 1) ? argv[1] : "xmodem.bin";
    shell_print(sh, "Starting XMODEM receive (destination: %s)...", out_path);
    int res = console_modem_rx_xmodem(out_path);
    if (res == 0) {
        shell_print(sh, "XMODEM transfer successful.");
    } else {
        shell_error(sh, "XMODEM transfer failed.");
    }
    return res;
}

static int cmd_modem_ry(const struct shell *sh, size_t argc, char **argv)
{
    const char *out_path = (argc > 1) ? argv[1] : NULL;
    shell_print(sh, "Starting YMODEM receive...");
    int res = console_modem_rx_ymodem(out_path);
    if (res == 0) {
        shell_print(sh, "YMODEM transfer successful.");
    } else {
        shell_error(sh, "YMODEM transfer failed.");
    }
    return res;
}

static int cmd_modem_rz(const struct shell *sh, size_t argc, char **argv)
{
    const char *out_path = (argc > 1) ? argv[1] : NULL;
    shell_print(sh, "Starting ZMODEM receive...");
    int res = console_modem_rx_zmodem(out_path);
    if (res == 0) {
        shell_print(sh, "ZMODEM transfer successful.");
    } else {
        shell_error(sh, "ZMODEM transfer failed.");
    }
    return res;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_modem,
    SHELL_CMD_ARG(rx, NULL, "Receive file using XMODEM: modem rx <file>", cmd_modem_rx, 1, 1),
    SHELL_CMD_ARG(ry, NULL, "Receive file using YMODEM: modem ry [file]", cmd_modem_ry, 1, 1),
    SHELL_CMD_ARG(rz, NULL, "Receive file using ZMODEM: modem rz [file]", cmd_modem_rz, 1, 1),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &sub_modem, "Serial transfer protocols (XMODEM/YMODEM/ZMODEM)", NULL);
SHELL_CMD_REGISTER(rx, NULL, "Alias for modem rx (XMODEM receive)", cmd_modem_rx);
SHELL_CMD_REGISTER(ry, NULL, "Alias for modem ry (YMODEM receive)", cmd_modem_ry);
SHELL_CMD_REGISTER(rz, NULL, "Alias for modem rz (ZMODEM receive)", cmd_modem_rz);

#endif

int zephyr_console_modem_init(void)
{
    return 0;
}
