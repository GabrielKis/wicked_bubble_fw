# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).
cmake_minimum_required(VERSION 3.22)

# Set board
set(BOARD my_esp32_devkitc_wroom/esp32/procpu)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Add zephyr base path
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Add project name
project(wicked_bubble_fw)

# How to add message on build
#message(STATUS "ZEPHYR_BASE = ${ZEPHYR_BASE}")

# Add include path
zephyr_include_directories(include)

target_sources(app PRIVATE
                src/main.c
                src/thread_msg/message_hmi_main.c
                src/thread_msg/message_main_bubble.c
                src/hmi/uart_cmd.c
                src/io_bubble/bubble_control.c
)

target_include_directories(app PRIVATE  src
                                        src/hmi
                                        src/io_bubble
                                        src/thread_msg
)
