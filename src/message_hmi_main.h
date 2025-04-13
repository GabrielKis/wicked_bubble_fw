#ifndef MSG_HMI_MAIN_H_
#define MSG_HMI_MAIN_H_

#include <stdint.h>
#include <zephyr/kernel.h>

enum hmi_cmd_type {
    HMI_CMD_SET_DUTY,
    HMI_CMD_SET_PERIOD,
    HMI_CMD_SET_SERVO_ANGLE,
    HMI_CMD_READ_SERVO_DATA
};

enum hmi_module_type {
    MOD_MAIN,
    MOD_SERVO
};

// HMI -> Main Message
struct hmi_msg_t {
    enum hmi_module_type module;
    enum hmi_cmd_type type;
    union {
        uint32_t duty;
        uint32_t period;
        uint32_t angle;
    } data;
};

int send_message_hmi_to_main(struct hmi_msg_t *hmi_data);
int recv_message_hmi_to_main(struct hmi_msg_t *hmi_data);

enum main_to_servo_cmd_type {
    MAIN_CMD_SET_DUTY,
    MAIN_CMD_SET_PERIOD,
    MAIN_CMD_SET_SERVO_ANGLE,
    MAIN_CMD_READ_SERVO_DATA,
    MAIN_CMD_MOV_DRAGON_HEAD
};

// Main -> Servo Message
struct main_to_servo_msg_t {
    enum main_to_servo_cmd_type type;
    union {
        uint32_t duty;
        uint32_t period;
        uint32_t angle;
    } data;
    bool start;
};

int send_message_main_to_servo(struct main_to_servo_msg_t *msg);
int recv_message_main_to_servo(struct main_to_servo_msg_t *msg);

#endif

