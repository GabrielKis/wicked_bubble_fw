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
    uint8_t module; // enum hmi_module_type
    uint8_t type;   // enum hmi_cmd_type
};

int send_message_hmi_to_main(struct hmi_msg_t *hmi_data);
int recv_message_hmi_to_main(struct hmi_msg_t *hmi_data);

#endif
