/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h> // Include this header for net_addr_pton

#include "message_iot_main.h"

LOG_MODULE_REGISTER(IOT_MQTT);

#define MQTT_BROKER_PORT 1883                // Default MQTT port
#define MQTT_CLIENT_ID "zephyr_client"       // Unique client ID
#define MQTT_TOPIC "bubble"        // Topic to subscribe to
#define MQTT_BUFFER_SIZE 128

static struct mqtt_client mqtt_client;
static struct sockaddr_storage broker;
static uint8_t rx_buffer[MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_BUFFER_SIZE];
static uint8_t payload_buf[MQTT_BUFFER_SIZE];
char gw_addr_str[NET_IPV4_ADDR_LEN];

enum mqtt_state {
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_SUBSCRIBE,
    MQTT_STATE_PROCESS,
    MQTT_STATE_IDLE,
};

static enum mqtt_state mqtt_state = MQTT_STATE_DISCONNECTED;

static void iot_message_control(uint8_t *cmd, uint8_t sz)
{
    struct iot_to_main_msg_t msg = {
        .type = IOT_CMD_BUBBLE_OFF,
    };

    if (cmd == NULL) {
        printk("Command unknown\n");
        return;
    }

    if (strcmp(cmd, "ON") == 0) {
        msg.type = IOT_CMD_BUBBLE_ON;
    } else if (strcmp(cmd, "OFF") == 0) {
        msg.type = IOT_CMD_BUBBLE_OFF;
    } else {
        printk("Command unknown\n");
        return;
    }

    if (send_message_iot_to_main(&msg) != 0) {
        printk("Failed to send duty command to queue\n");
    }
}


static void mqtt_event_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    LOG_INF("MQTT EVENT event received: %d", evt->type);

    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result == 0) {
            LOG_INF("MQTT EVENT connected");
            //mqtt_state = MQTT_STATE_CONNECTED;
        } else {
            LOG_ERR("MQTT EVENT connection failed: %d", evt->result);
            mqtt_state = MQTT_STATE_DISCONNECTED;
        }
        break;

    case MQTT_EVT_DISCONNECT:
        LOG_INF("MQTT disconnected");
        mqtt_state = MQTT_STATE_DISCONNECTED;
        break;

    case MQTT_EVT_PUBLISH: {
        const struct mqtt_publish_param *p = &evt->param.publish;
        int len = p->message.payload.len < MQTT_BUFFER_SIZE ? p->message.payload.len : MQTT_BUFFER_SIZE - 1;

        mqtt_read_publish_payload(client, payload_buf, len);
        payload_buf[len] = '\0'; // Null-terminate the payload
        iot_message_control(payload_buf, len);
        LOG_INF("Received message on topic %s: %s", p->message.topic.topic.utf8, payload_buf);
        break;
    }

    case MQTT_EVT_SUBACK:
        LOG_INF("Subscribed to topic %s", MQTT_TOPIC);
        break;

    default:
        LOG_INF("Unhandled MQTT event: %d", evt->type);
        break;
    }
}

static int iot_mqtt_connect()
{
    struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

    static bool once = false;
    if (!once) {
        mqtt_client_init(&mqtt_client);
        once = true;
    }

    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(MQTT_BROKER_PORT);

    if (net_addr_pton(AF_INET, gw_addr_str, &broker4->sin_addr) != 0) {
        LOG_ERR("Failed to parse broker address");
        return -EINVAL;
    }

    mqtt_client.broker = &broker;
    mqtt_client.evt_cb = mqtt_event_handler;
    mqtt_client.client_id.utf8 = MQTT_CLIENT_ID;
    mqtt_client.client_id.size = strlen(MQTT_CLIENT_ID);
    mqtt_client.password = NULL;
    mqtt_client.user_name = NULL;
    mqtt_client.protocol_version = MQTT_VERSION_3_1_1;
    mqtt_client.transport.type = MQTT_TRANSPORT_NON_SECURE;

    mqtt_client.rx_buf = rx_buffer;
    mqtt_client.rx_buf_size = sizeof(rx_buffer);
    mqtt_client.tx_buf = tx_buffer;
    mqtt_client.tx_buf_size = sizeof(tx_buffer);

    // LOG_INF("MQTT Client Connect %s", mqtt_client.client_id.utf8);
    // LOG_INF("MQTT Broker %s", MQTT_BROKER_HOSTNAME);

    int ret = mqtt_connect(&mqtt_client);
    if (ret != 0) {
        LOG_ERR("MQTT connect failed: %d", ret);
    } else {
        LOG_INF("MQTT client connected to broker");
    }
    return ret;
}

int iot_mqtt_subscribe(void)
{
    int ret = mqtt_input(&mqtt_client);
    if (ret != 0) {
        LOG_ERR("mqtt_input failed: %d", ret);
        return ret;
    }

    struct mqtt_topic topic = {
        .topic = {
            .utf8 = MQTT_TOPIC,
            .size = strlen(MQTT_TOPIC),
        },
        .qos = MQTT_QOS_0_AT_MOST_ONCE,
    };

    struct mqtt_subscription_list sub_list = {
        .list = &topic,
        .list_count = 1,
        .message_id = 1,
    };

    LOG_INF("Subscribing to topic: %s with QoS: %d", MQTT_TOPIC, topic.qos);

    ret = mqtt_subscribe(&mqtt_client, &sub_list);
    if (ret != 0) {
        LOG_ERR("Failed to subscribe to topic: %d", ret);
    }

    return ret;
}

char* get_gw_addr_ptr()
{
    return gw_addr_str;
}

void mqtt_process(void)
{
    int ret;

    ret = mqtt_input(&mqtt_client);
    if (ret != 0) {
        LOG_ERR("mqtt_input failed: %d", ret);
        return;
    }

    ret = mqtt_live(&mqtt_client);
    if (ret != 0 && ret != -EAGAIN) {
        LOG_ERR("mqtt_live failed: %d", ret);
    }
}

void mqtt_state_machine()
{
    switch (mqtt_state) {
    case MQTT_STATE_DISCONNECTED:
        LOG_INF("MQTT state: DISCONNECTED");
        if (iot_mqtt_connect() == 0) {
            mqtt_state = MQTT_STATE_SUBSCRIBE;
        }
        break;
    case MQTT_STATE_CONNECTED:
        LOG_INF("MQTT state: CONNECTED ✅");
        mqtt_process();
        break;
    case MQTT_STATE_SUBSCRIBE:
        // Check for incoming messages or other events
        LOG_INF("MQTT state: SUBSCRIBE");
        if (iot_mqtt_subscribe() == 0) {
            mqtt_state = MQTT_STATE_CONNECTED; // Move to PROCESS state after subscribing
        }
        break;
    case MQTT_STATE_PROCESS:
        // Check for incoming messages or other events
        LOG_INF("MQTT state: PROCESS");
        break;
    case MQTT_STATE_IDLE:
        // Check for incoming messages or other events
        LOG_INF("MQTT state: IDLE");
        break;
    default:
        LOG_ERR("Unknown MQTT state: %d", mqtt_state);
        break;
    }
    k_msleep(1000); // Sleep for 1 second before checking the state again
}