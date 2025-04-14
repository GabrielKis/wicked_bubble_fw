/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "message_hmi_main.h"

#define UART_THREAD_STACK_SIZE 1024
#define UART_THREAD_PRIORITY 7

#define RING_BUF_SIZE 64

#define UART_CMD_ARRAY_SZ   3U
#define UART_ARRAY_SZ       20U

// Thread initialization
K_THREAD_STACK_DEFINE(uart_thread_stack, UART_THREAD_STACK_SIZE);
struct k_thread uart_thread_data;
k_tid_t uart_thread_id;

// UART and ring buffer initialization
RING_BUF_DECLARE(uart_ringbuf, RING_BUF_SIZE);
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
struct UART_Command
{
    uint8_t size;
    uint8_t header[2];
    uint8_t header_size;
    void (*func)(uint8_t*, uint8_t);
    char *help_text;
};

static void uart_cmd_bubble_control(uint8_t *cmd, uint8_t sz);
static void uart_cmd_help_message(uint8_t *cmd, uint8_t sz);

static struct UART_Command commands_list[UART_CMD_ARRAY_SZ] = {
    {.size = 1, .header = {'1'}, .header_size = 1, .func = uart_cmd_bubble_control, .help_text = "1: Turn on Bubble\n"},
    {.size = 1, .header = {'2'}, .header_size = 1, .func = uart_cmd_bubble_control, .help_text = "2: Turn off Bubble\n"},
    {.size = 1, .header = {'0'}, .header_size = 1, .func = uart_cmd_help_message, .help_text = NULL},
};

static void uart_cmd_bubble_control(uint8_t *cmd, uint8_t sz)
{
    struct hmi_msg_t msg = {
        .type = HMI_CMD_BUBBLE_ON,
        .module = MOD_BUBBLE,
    };

    if (cmd == NULL) {
        printk("Command unknown\n");
        return;
    }

    if (cmd[0] == '1') {
        msg.type = HMI_CMD_BUBBLE_ON;
    } else if (cmd[0] == '2') {
        msg.type = HMI_CMD_BUBBLE_OFF;
    } else {
        printk("Command unknown\n");
        return;
    }

    if (send_message_hmi_to_main(&msg) != 0) {
        printk("Failed to send duty command to queue\n");
    }
}

static void uart_cmd_help_message(uint8_t *cmd, uint8_t sz)
{
    printk("=========================\n");
    printk("===== UART Commands =====\n");
    printk("=========================\n");
    for (uint8_t i=0; i<UART_CMD_ARRAY_SZ; i++) {
        if (commands_list[i].help_text == NULL) {
            break;
        }
        printk("%s", commands_list[i].help_text);
    }
}

// -----------------------------------
// Static functions
// -----------------------------------
static void process_uart_data(uint8_t *data, uint8_t size)
{
    // Parse command: iterate over data cmd list to find matching command
    bool command_found = false;
    for (uint8_t i=0; i<UART_CMD_ARRAY_SZ; i++) {
        if (size < commands_list[i].header_size) {
            // not this command
            printf("here\n");
            continue;
        }

        if ((memcmp(data, commands_list[i].header, commands_list[i].header_size) == 0) && (size == commands_list[i].size)) {
            // command found
            command_found = true;
            commands_list[i].func(data, size);
            break;
        }
    }

    if (!command_found) {
        printk("Command not found: \"%s\"\n", data);
    }
}

static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t received_byte;
    if (!uart_irq_update(dev)) {
        return;
    }

    while (uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, &received_byte, 1);
        ring_buf_put(&uart_ringbuf, &received_byte, 1);
    }
}

static void uart_thread_entry(void *p1, void *p2, void *p3)
{
    printk("UART Thread Started\n");

    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
    uart_irq_rx_enable(uart_dev);

    uint8_t data;
    uint8_t cmd_buffer[UART_ARRAY_SZ];
    uint8_t cmd_index = 0;
    bool skip_next = false;
    while (1) {
        while (ring_buf_get(&uart_ringbuf, &data, 1) > 0) {
            // Echo character back
            uart_poll_out(uart_dev, data);

            // Check for line endings
            if (data == '\n' || data == '\r') {
                if (data == '\r') {
                    skip_next = true;  // Skip \n if it follows \r
                } else if (skip_next) {
                    skip_next = false;
                    continue;
                }

                // Process non-empty commands
                if (cmd_index > 0) {
                    cmd_buffer[cmd_index] = '\0';
                    process_uart_data(cmd_buffer, cmd_index);
                    cmd_index = 0;
                }
            } else if (cmd_index < UART_ARRAY_SZ - 1) {
                cmd_buffer[cmd_index++] = data;
                skip_next = false;
            } else {
                printk("Command too long, buffer cleared\n");
                cmd_index = 0;
                skip_next = false;
            }
        }

        k_msleep(10);
    }
}

// -----------------------------------
// Public functions
// -----------------------------------
void uart_thread_start(void)
{
    uart_thread_id = k_thread_create(&uart_thread_data, uart_thread_stack,
                                     K_THREAD_STACK_SIZEOF(uart_thread_stack),
                                     uart_thread_entry, NULL, NULL, NULL,
                                     UART_THREAD_PRIORITY, 0, K_NO_WAIT);
    printk("UART thread created\n");
}