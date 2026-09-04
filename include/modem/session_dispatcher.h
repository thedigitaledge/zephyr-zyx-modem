/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Multi-Session Transport Dispatcher & Multiplexing for Zephyr OS.
 */

#ifndef MODEM_SESSION_DISPATCHER_H_
#define MODEM_SESSION_DISPATCHER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DISPATCH_SESSIONS 4

/**
 * @brief Session Channel Types
 */
typedef enum {
    MODEM_CHANNEL_UART = 0,
    MODEM_CHANNEL_USB_CDC_ACM,
    MODEM_CHANNEL_BLE_NUS,
    MODEM_CHANNEL_SOCKET,
    MODEM_CHANNEL_NFC
} modem_channel_type_t;

/**
 * @brief Session Dispatcher Context
 */
typedef struct {
    int session_id;
    modem_channel_type_t channel_type;
    bool active;
    size_t bytes_transferred;
} modem_session_t;

/**
 * @brief Initialize multi-session transport dispatcher.
 * @return 0 on success.
 */
int session_dispatcher_init(void);

/**
 * @brief Create and register a new transfer session.
 * @param type Channel type for session.
 * @return Session ID (>= 0) on success, negative error code on failure.
 */
int session_dispatcher_create(modem_channel_type_t type);

/**
 * @brief Close active session.
 * @param session_id Session ID to close.
 * @return 0 on success, negative error code on failure.
 */
int session_dispatcher_close(int session_id);

/**
 * @brief Get total active session count.
 * @return Active session count.
 */
size_t session_dispatcher_get_active_count(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_SESSION_DISPATCHER_H_ */
