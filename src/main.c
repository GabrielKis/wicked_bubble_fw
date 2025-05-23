/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <app_version.h>

#include "uart_cmd.h"
#include "bubble_control.h"
#include "iot_thread.h"

#include "message_hmi_main.h"
#include "message_main_bubble.h"
#include "message_iot_main.h"

void handle_hmi_msg(void)
{
    static struct hmi_msg_t hmi_msg = {};
    if (recv_message_hmi_to_main(&hmi_msg) != 0) {
        return;
    }

    switch (hmi_msg.module) {
        case MOD_MAIN:
            //printk("HMI Command to Main Thread\n");
            break;
        case MOD_BUBBLE:
            //printk("HMI Command to Bubble Thread\n");
            struct main_to_bubble_msg_t main_to_bubble = {0};
            if (hmi_msg.type == HMI_CMD_BUBBLE_ON) {
                main_to_bubble.type = MAIN_CMD_BUBBLE_ON;
            } else if (hmi_msg.type == HMI_CMD_BUBBLE_OFF) {
                main_to_bubble.type = MAIN_CMD_BUBBLE_OFF;
            } else {
                return;
            }
            send_message_main_to_bubble(&main_to_bubble);
            //TODO: Handle return from message sent
            break;
        default:
            printk("Command not recognized from HMI: %02X", hmi_msg.type);
            break;
    }
}

void handle_iot_msg(void)
{
    static struct iot_to_main_msg_t iot_msg = {};
    if (recv_message_iot_to_main(&iot_msg) != 0) {
        return;
    }

    struct main_to_bubble_msg_t main_to_bubble = {0};
    switch (iot_msg.type) {
        case IOT_CMD_BUBBLE_ON:
            main_to_bubble.type = MAIN_CMD_BUBBLE_ON;
            //printk("HMI Command to Main Thread\n");
            break;
        case IOT_CMD_BUBBLE_OFF:
            main_to_bubble.type = MAIN_CMD_BUBBLE_OFF;
            break;
        default:
            printk("Command not recognized from IOT: %02X", iot_msg.type);
            return;
    }
    send_message_main_to_bubble(&main_to_bubble);
}

int main(void)
{
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    bubble_thread_start();
    //uart_thread_start();
    iot_thread_start();

    while (1) {
        // Handle communication with all threads
        //handle_hmi_msg();
        handle_iot_msg();
        // Read data from HMI Thread
        // Read data from Wifi/MQTT
        // Write data to Servo Thread
        // Read data from servo thread
        k_msleep(50);  // Delay to avoid CPU usage
    }

    return 0;
}
