# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Christopher West <cwest@thedigitaledge.co.uk>

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(multi_file_download_ymodem)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../include ${CMAKE_CURRENT_LIST_DIR}/../../src)
target_sources(app PRIVATE src/main.c ../../src/crc.c ../../src/ymodem.c)
