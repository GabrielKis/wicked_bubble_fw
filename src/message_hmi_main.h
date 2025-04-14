#ifndef MSG_HMI_MAIN_H_
#define MSG_HMI_MAIN_H_

#include <stdint.h>
#include <zephyr/kernel.h>

enum hmi_cmd_type {
    HMI_CMD_BUBBLE_ON,
    HMI_CMD_BUBBLE_OFF,
};

enum hmi_module_type {
    MOD_MAIN,
    MOD_BUBBLE
};

// HMI -> Main Message
struct hmi_msg_t {
    enum hmi_module_type module;
    enum hmi_cmd_type type;
};

int send_message_hmi_to_main(struct hmi_msg_t *hmi_data);
int recv_message_hmi_to_main(struct hmi_msg_t *hmi_data);

// TODO: Move this below to another file and create an message folder containing messages data
enum main_to_bubble_cmd_type {
    MAIN_CMD_BUBBLE_ON,
    MAIN_CMD_BUBBLE_OFF,
};

// Main -> Servo Message
struct main_to_bubble_msg_t {
    enum main_to_bubble_cmd_type type;
};

int send_message_main_to_bubble(struct main_to_bubble_msg_t *msg);
int recv_message_main_to_bubble(struct main_to_bubble_msg_t *msg);

#endif
