#ifndef IOT_MAIN_BUBBLE_H_
#define IOT_MAIN_BUBBLE_H_

#include <stdint.h>
#include <zephyr/kernel.h>

enum iot_to_main_cmd_type {
    IOT_CMD_BUBBLE_ON,
    IOT_CMD_BUBBLE_OFF,
};

struct iot_to_main_msg_t {
    uint8_t type; // Values from "iot_to_main_cmd_type"
};

int send_message_iot_to_main(struct iot_to_main_msg_t *msg);
int recv_message_iot_to_main(struct iot_to_main_msg_t *msg);

#endif
