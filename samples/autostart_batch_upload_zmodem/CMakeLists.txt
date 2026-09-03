# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(autostart_batch_upload_zmodem)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../include ${CMAKE_CURRENT_LIST_DIR}/../../src)
target_sources(app PRIVATE src/main.c)
