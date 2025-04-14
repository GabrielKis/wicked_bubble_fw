/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "message_hmi_main.h"

K_MSGQ_DEFINE(hmi_msgq, sizeof(struct hmi_msg_t), 5, 4);

// HMI -> Main Message
int send_message_hmi_to_main(struct hmi_msg_t *hmi_data) {
    return k_msgq_put(&hmi_msgq, hmi_data, K_NO_WAIT);
}
int recv_message_hmi_to_main(struct hmi_msg_t *hmi_data) {
    return k_msgq_get(&hmi_msgq, hmi_data, K_NO_WAIT);
}
