/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of BSD Network Socket Stream Modem Transport Adapter.
 */

#include <modem/net_socket_transport.h>

static int active_sock_fd = -1;

int net_socket_transport_init(const net_socket_transport_config_t *config)
{
    if (!config) {
        return -1;
    }
    active_sock_fd = config->socket_fd;
    return 0;
}

int net_socket_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data)
{
    (void)timeout_ms;
    (void)user_data;
    if (!byte || active_sock_fd < 0) {
        return -1;
    }
    /* Simulated read logic for test/socket integration */
    return -1;
}

int net_socket_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data)
{
    (void)user_data;
    if (!buf || len == 0 || active_sock_fd < 0) {
        return -1;
    }
    return 0;
}

int net_socket_transport_close(void *user_data)
{
    (void)user_data;
    active_sock_fd = -1;
    return 0;
}
