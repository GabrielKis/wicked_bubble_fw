/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "message_main_bubble.h"

/* PWM device for servo control */
#define BUBBLE_THREAD_STACK_SIZE 1024
#define BUBBLE_THREAD_PRIORITY 7

#if !DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#error "LED0 GPIO node is not ready"
#endif

static const struct gpio_dt_spec bubble_io = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// Thread initialization
K_THREAD_STACK_DEFINE(bubble_thread_stack, BUBBLE_THREAD_STACK_SIZE);
struct k_thread bubble_thread_data;
k_tid_t bubble_thread_id;

// -----------------------------------
// Static functions
// -----------------------------------
static void init_io(void)
{
    if (!device_is_ready(bubble_io.port)) {
        printk("Error: IO device %s is not ready\n", bubble_io.port->name);
        return;
    }

    int ret = gpio_pin_configure_dt(&bubble_io, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
    }

    gpio_pin_set_dt(&bubble_io, 0);
}

void handle_main_msg(void)
{
    static struct main_to_bubble_msg_t msg = {};
    if (recv_message_main_to_bubble(&msg) != 0) {
        return;
    }

    switch (msg.type) {
        case MAIN_CMD_BUBBLE_ON:
            printk("BUBBLE: On\n");
            gpio_pin_set_dt(&bubble_io, 1);
            break;
        case MAIN_CMD_BUBBLE_OFF:
            printk("BUBBLE: Off\n");
            gpio_pin_set_dt(&bubble_io, 0);
            break;
        default:
            printk("Unknown HMI Command: %d\n", msg.type);
            break;
    }
}

static void bubble_thread_entry(void *p1, void *p2, void *p3)
{
    init_io();

    while (1) {
        handle_main_msg();
        k_msleep(10);
    }

    return;
}

// -----------------------------------
// Public functions
// -----------------------------------
void bubble_thread_start(void)
{
    bubble_thread_id = k_thread_create(&bubble_thread_data, bubble_thread_stack,
                                    K_THREAD_STACK_SIZEOF(bubble_thread_stack),
                                    bubble_thread_entry, NULL, NULL, NULL,
                                    BUBBLE_THREAD_PRIORITY, 0, K_NO_WAIT);
    printk("BUBBLE thread created\n");
}
