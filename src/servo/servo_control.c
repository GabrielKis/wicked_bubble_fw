/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#include "message_hmi_main.h"

/* PWM device for servo control */
#define PWM_SERVO_DEFAULT_PERIOD_US 20000     // 20ms (50Hz)
#define PWM_SERVO_DEFAULT_DUTY      100         // 50,0 %

#define PWM_SERVO_DUTY_PERCENT      1000U
#define PWM_SERVO_PERIOD_TO_US      1000U

#define PWM_SERVO_MAX_DUTY          1000U
#define PWM_SERVO_MAX_PERIOD_MS     1000U
#define PWM_SERVO_MAX_ANGLE         180U

#define SERVO_THREAD_STACK_SIZE 1024
#define SERVO_THREAD_PRIORITY 7

#if !DT_NODE_HAS_STATUS(DT_ALIAS(pwm0), okay)
#error "LED0 GPIO node is not ready"
#endif

static const struct pwm_dt_spec servo = PWM_DT_SPEC_GET(DT_ALIAS(pwm0));

// Thread initialization
K_THREAD_STACK_DEFINE(servo_thread_stack, SERVO_THREAD_STACK_SIZE);
struct k_thread servo_thread_data;
k_tid_t servo_thread_id;

struct servo_control_data_t {
    uint32_t period_us;
    uint32_t duty; // 0 - 100.0 %
};

static struct servo_control_data_t servo_data = {
    .period_us = PWM_SERVO_DEFAULT_PERIOD_US,
    .duty = PWM_SERVO_DEFAULT_DUTY,
};

// -----------------------------------
// Static functions
// -----------------------------------
static void init_servo(void)
{
    int ret;
    if (!device_is_ready(servo.dev)) {
        printk("Error: PWM device %s is not ready\n", servo.dev->name);
        return;
    }

    // Set PWM Servo to initial value
    ret = pwm_set(servo.dev, 0, PWM_USEC(servo_data.period_us), PWM_USEC(servo_data.period_us*servo_data.duty/PWM_SERVO_DUTY_PERCENT), 0);
    if (ret < 0) {
        printk("Error %d: failed to set pulse width\n", ret);
        return;
    }
    return;
}

bool servo_set_duty(uint32_t duty_value)
{
    if (duty_value > PWM_SERVO_MAX_DUTY) {
        printk("servo_set_duty: duty_value %u exceeds max %u\n", duty_value, PWM_SERVO_MAX_DUTY);
        return false;
    }

    servo_data.duty = duty_value;
    return true;
}

bool servo_set_period(uint32_t period)
{
    if (period > PWM_SERVO_MAX_PERIOD_MS) {
        printk("servo_set_period: period %u ms exceeds max %u ms\n", period, PWM_SERVO_MAX_PERIOD_MS);
        return false;
    }

    servo_data.period_us = period * PWM_SERVO_PERIOD_TO_US;
    return true;
}

bool servo_set_angle(uint32_t angle)
{
    if (angle > PWM_SERVO_MAX_ANGLE) {
        printk("servo_set_angle: angle %u° exceeds max %u°\n", angle, PWM_SERVO_MAX_ANGLE);
        return false;
    }

    //TODO: Implement angle conversion to pwm period
    return true;
}

void read_servo_data(void)
{
    printk("Servo motor info:\n");
    printk("Frequency:  %dHz\n", 1000000/servo_data.period_us);
    printk("Period:     %dus\n", servo_data.period_us);
    printk("Duty cycle: %d,%d%% \n", servo_data.duty/10, servo_data.duty%10);
    printk("Angle:      %d degrees\n", 0);
}

void handle_main_msg(void)
{
    static struct main_to_servo_msg_t msg = {};
    bool value_updated = false;
    int ret;
    if (recv_message_main_to_servo(&msg) != 0) {
        return;
    }

    switch (msg.type) {
        case MAIN_CMD_SET_DUTY:
            printk("SERVO: Set duty of %d,%d%%\n", msg.data.duty/10, msg.data.duty%10);
            value_updated = servo_set_duty(msg.data.duty);
            break;
        case MAIN_CMD_SET_PERIOD:
            printk("SERVO: Set period of %dms\n", msg.data.period);
            value_updated = servo_set_period(msg.data.period);
            break;
        case MAIN_CMD_SET_SERVO_ANGLE:
            printk("SERVO: Set angle of %d°\n", msg.data.angle);
            value_updated = true;
            break;
        case MAIN_CMD_READ_SERVO_DATA:
            printk("HMI Command to Main Thread\n"); 
            read_servo_data();
            break;
        default:
            printk("Unknown HMI Command: %d\n", msg.type);
            break;
    }

    if (value_updated) {
        uint32_t a, b;
        a = PWM_USEC(servo_data.period_us);
        b = PWM_USEC((servo_data.period_us/PWM_SERVO_DUTY_PERCENT)*servo_data.duty);
        ret = pwm_set(servo.dev, 0, a, b, 0);
        if (ret < 0) {
            printk("Error %d: failed to set pulse width\n", ret);
            return;
        }
    }
}

static void servo_thread_entry(void *p1, void *p2, void *p3)
{
    init_servo();

    while (1) {
        handle_main_msg();
        k_msleep(10);
    }

    return;
}

// -----------------------------------
// Public functions
// -----------------------------------
void servo_thread_start(void)
{
    servo_thread_id = k_thread_create(&servo_thread_data, servo_thread_stack,
                                    K_THREAD_STACK_SIZEOF(servo_thread_stack),
                                    servo_thread_entry, NULL, NULL, NULL,
                                    SERVO_THREAD_PRIORITY, 0, K_NO_WAIT);
    printk("SERVO thread created\n");
}
