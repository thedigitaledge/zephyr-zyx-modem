/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Implementation of Multi-Session Transport Dispatcher & Multiplexing for Zephyr OS.
 */

#include <modem/session_dispatcher.h>

static modem_session_t sessions[MAX_DISPATCH_SESSIONS];

int session_dispatcher_init(void)
{
    for (int i = 0; i < MAX_DISPATCH_SESSIONS; i++) {
        sessions[i].session_id = i;
        sessions[i].channel_type = MODEM_CHANNEL_UART;
        sessions[i].active = false;
        sessions[i].bytes_transferred = 0;
    }
    return 0;
}

int session_dispatcher_create(modem_channel_type_t type)
{
    for (int i = 0; i < MAX_DISPATCH_SESSIONS; i++) {
        if (!sessions[i].active) {
            sessions[i].active = true;
            sessions[i].channel_type = type;
            sessions[i].bytes_transferred = 0;
            return sessions[i].session_id;
        }
    }
    return -1;
}

int session_dispatcher_close(int session_id)
{
    if (session_id < 0 || session_id >= MAX_DISPATCH_SESSIONS) {
        return -1;
    }
    sessions[session_id].active = false;
    return 0;
}

size_t session_dispatcher_get_active_count(void)
{
    size_t count = 0;
    for (int i = 0; i < MAX_DISPATCH_SESSIONS; i++) {
        if (sessions[i].active) {
            count++;
        }
    }
    return count;
}
