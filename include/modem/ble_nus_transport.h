/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>
 *
 * Header file for Bluetooth Low Energy (BLE) Nordic UART Service (NUS)
 * modem transport adapter for Zephyr OS.
 */

#ifndef MODEM_BLE_NUS_TRANSPORT_H_
#define MODEM_BLE_NUS_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE NUS Transport Configuration Structure
 */
typedef struct {
    size_t rx_ring_buffer_size; /**< Size of internal RX ring buffer */
    uint32_t conn_timeout_ms;   /**< BLE connection timeout in ms */
} ble_nus_transport_config_t;

/**
 * @brief Initialize BLE NUS Transport Adapter.
 * @param config Pointer to configuration parameters (NULL for defaults).
 * @return 0 on success, negative error code on failure.
 */
int ble_nus_transport_init(const ble_nus_transport_config_t *config);

/**
 * @brief Read byte from BLE NUS RX ring buffer.
 * @param byte Output pointer for received byte.
 * @param timeout_ms Timeout in milliseconds to wait for data.
 * @param user_data User context pointer (unused/optional).
 * @return 0 on success, -1 on timeout/error, -2 on cancellation.
 */
int ble_nus_transport_read_byte(uint8_t *byte, uint32_t timeout_ms, void *user_data);

/**
 * @brief Write bytes over BLE NUS notification stream.
 * @param buf Data buffer to send.
 * @param len Length of data in bytes.
 * @param user_data User context pointer (unused/optional).
 * @return 0 on success, negative error code on failure.
 */
int ble_nus_transport_write_bytes(const uint8_t *buf, size_t len, void *user_data);

/**
 * @brief Receive handler callback for incoming BLE NUS data.
 * @param data Received bytes buffer.
 * @param len Received data length.
 */
void ble_nus_transport_rx_callback(const uint8_t *data, size_t len);

/**
 * @brief Check if BLE NUS peer is currently connected.
 * @return true if connected, false otherwise.
 */
bool ble_nus_transport_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_BLE_NUS_TRANSPORT_H_ */
