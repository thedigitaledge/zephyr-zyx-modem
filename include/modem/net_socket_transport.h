/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for BSD Network Socket Stream Modem Transport Adapter for Zephyr OS.
 */

#ifndef MODEM_NET_SOCKET_TRANSPORT_H_
#define MODEM_NET_SOCKET_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network Socket Transport Configuration
 */
typedef struct {
    int socket_fd;           /**< Socket file descriptor */
    uint32_t read_timeout_ms;/**< Read timeout in ms */
} net_socket_transport_config_t;

/**
 * @brief Initialize Network Socket Transport Adapter.
 * @param config Socket transport configuration settings.
 * @return 0 on success, negative error code on failure.
 */
int net_socket_transport_init(const net_socket_transport_config_t *config);

/**
 * @brief Read byte from network socket stream.
 * @param byte Output pointer for received byte.
 * @param timeout_ms Timeout in milliseconds.
 * @param user_data User context pointer (or socket config).
 * @return 0 on success, -1 on timeout/error, -2 on cancellation.
 */
int net_socket_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);

/**
 * @brief Write bytes to network socket stream.
 * @param buf Buffer containing data to transmit.
 * @param len Length of buffer in bytes.
 * @param user_data User context pointer (or socket config).
 * @return 0 on success, negative error code on failure.
 */
int net_socket_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief Close socket transport connection.
 * @param user_data Socket configuration context.
 * @return 0 on success.
 */
int net_socket_transport_close(void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_NET_SOCKET_TRANSPORT_H_ */
