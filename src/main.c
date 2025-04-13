/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <app_version.h>
#include "uart_cmd.h"
#include "servo_control.h"
#include "message_hmi_main.h"

// Assuming you have defined your message queue or other IPC mechanism
// For example, using a message queue:

void handle_hmi_msg(void)
{
    static struct hmi_msg_t hmi_msg = {};
    if (recv_message_hmi_to_main(&hmi_msg) != 0) {
        return;
    }

    switch (hmi_msg.module) {
        case MOD_MAIN:
            printk("HMI Command to Main Thread\n"); 
            break;
        case MOD_SERVO:
            printk("HMI Command to Servo Thread\n");
            struct main_to_servo_msg_t main_to_servo = {};
            //prepare_main_to_servo_msg(&hmi_msg, &main_to_servo);
            main_to_servo.type = hmi_msg.type;
            main_to_servo.data.duty = hmi_msg.data.duty;
            send_message_main_to_servo(&main_to_servo);
            //TODO: Handle return from message sent
            break;
        default:
            printk("Command not recognized from HMI: %02X", hmi_msg.type);
            break;
    }
}

int main(void)
{
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    servo_thread_start();
    uart_thread_start();

    while (1) {
        // Handle communication with all threads
        handle_hmi_msg();
        // Read data from HMI Thread
        // Read data from Wifi/MQTT
        // Write data to Servo Thread
        // Read data from servo thread
        k_msleep(50);  // Delay to avoid CPU usage
    }

    return 0;
}
