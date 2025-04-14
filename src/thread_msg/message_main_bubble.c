/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "message_main_bubble.h"

K_MSGQ_DEFINE(main_to_bubble_msgq, sizeof(struct main_to_bubble_msg_t), 5, 4);

// Main -> Bubble IO Message
int send_message_main_to_bubble(struct main_to_bubble_msg_t *msg) {
    return k_msgq_put(&main_to_bubble_msgq, msg, K_NO_WAIT);
}
int recv_message_main_to_bubble(struct main_to_bubble_msg_t *msg) {
    return k_msgq_get(&main_to_bubble_msgq, msg, K_NO_WAIT);
}
