#ifndef MSG_MAIN_BUBBLE_H_
#define MSG_MAIN_BUBBLE_H_

#include <stdint.h>
#include <zephyr/kernel.h>

// TODO: Move this below to another file and create an message folder containing messages data
enum main_to_bubble_cmd_type {
    MAIN_CMD_BUBBLE_ON,
    MAIN_CMD_BUBBLE_OFF,
};

// Main -> Servo Message
struct main_to_bubble_msg_t {
    uint8_t type; // Values from "main_to_bubble_cmd_type"
};

int send_message_main_to_bubble(struct main_to_bubble_msg_t *msg);
int recv_message_main_to_bubble(struct main_to_bubble_msg_t *msg);

#endif
