/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "message_iot_main.h"

K_MSGQ_DEFINE(iot_to_main_msgq, sizeof(struct iot_to_main_msg_t), 5, 4);

// Main -> Bubble IO Message
int send_message_iot_to_main(struct iot_to_main_msg_t *msg) {
    return k_msgq_put(&iot_to_main_msgq, msg, K_NO_WAIT);
}
int recv_message_iot_to_main(struct iot_to_main_msg_t *msg) {
    return k_msgq_get(&iot_to_main_msgq, msg, K_NO_WAIT);
}
