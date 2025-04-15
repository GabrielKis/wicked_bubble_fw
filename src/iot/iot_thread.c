/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>

#include "iot_mqtt.h"
// #include <zephyr/net/mqtt.h>
// #include <zephyr/net/socket.h>
// #include <zephyr/random/random.h>

#define IOT_THREAD_STACK_SIZE 2048 // Increase from 1024 to 2048
#define IOT_THREAD_PRIORITY 7

// Thread initialization
K_THREAD_STACK_DEFINE(iot_thread_stack, IOT_THREAD_STACK_SIZE);
struct k_thread iot_thread_data;
k_tid_t iot_thread_id;

LOG_MODULE_REGISTER(IOT);

// #define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                                                        \
	(NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT |                        \
	 NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT |                      \
	 NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

/* STA Mode Configuration */
#define WIFI_SSID "CCEM_BOLHA"
#define WIFI_PSK  "R3N4T4!10"

static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

enum state_wifi {
    IOT_STATE_DISCONNECTED,
    IOT_STATE_CONNECTING,
    IOT_STATE_CONNECTED,
    IOT_STATE_MQTT_INIT,
    IOT_STATE_MQTT_EVENTS,
};
static enum state_wifi wifi_state = IOT_STATE_DISCONNECTED;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
                    struct net_if *iface)
{
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: {
        LOG_INF("EVENT: Connection result: %s", WIFI_SSID);
        wifi_state = IOT_STATE_CONNECTED;
        break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT: {
        LOG_INF("EVENT: Disconnection result %s", WIFI_SSID);
        wifi_state = IOT_STATE_DISCONNECTED;
        break;
    }
    case NET_EVENT_WIFI_AP_STA_CONNECTED: {
        LOG_INF("EVENT: AP STA Connected");
        break;
    }
    case NET_EVENT_WIFI_AP_STA_DISCONNECTED: {
        LOG_INF("EVENT: AP STA Disconnected");
        break;
    }
    default:
        break;
    }
}

static void connect_to_wifi(void)
{
    if (!sta_iface) {
        LOG_INF("STA: interface no initialized");
        return;
    }

    sta_config.ssid = (const uint8_t *)WIFI_SSID;
    sta_config.ssid_length = strlen(WIFI_SSID);
    sta_config.psk = (const uint8_t *)WIFI_PSK;
    sta_config.psk_length = strlen(WIFI_PSK);
    sta_config.security = WIFI_SECURITY_TYPE_PSK;
    sta_config.channel = WIFI_CHANNEL_ANY;
    sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

    LOG_INF("Connecting to SSID: %s", sta_config.ssid);

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
                sizeof(struct wifi_connect_req_params));
    if (ret) {
        LOG_ERR("Unable to Connect to (%s)", WIFI_SSID);
    }
}

static void iot_thread_entry(void *p1, void *p2, void *p3)
{
    k_sleep(K_SECONDS(5));

    net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&cb);

    /* Get STA interface in AP-STA mode. */
    sta_iface = net_if_get_wifi_sta();

    struct in_addr gw_addr = {};
    char* gw_ip_str = get_gw_addr_ptr();

    while (1) {
        switch (wifi_state) {
            case IOT_STATE_DISCONNECTED:
                connect_to_wifi();
                wifi_state = IOT_STATE_CONNECTING;
                k_msleep(1000);
                break;
            case IOT_STATE_CONNECTING:
                LOG_INF("Wi-Fi connecting...");
                k_msleep(1000);
                break;
            case IOT_STATE_CONNECTED:
                gw_addr = net_if_ipv4_get_gw(sta_iface);
                if (gw_addr.s_addr != 0) { // Check if the gateway address is valid
                    net_addr_ntop(AF_INET, &gw_addr, gw_ip_str, sizeof(gw_ip_str));
                    wifi_state = IOT_STATE_MQTT_INIT;
                    LOG_INF("Gateway IP: %s", gw_ip_str);
                } else {
                    LOG_ERR("No gateway address assigned");
                }
                k_msleep(1000);
                break;
            case IOT_STATE_MQTT_INIT:
                mqtt_state_machine(); // Initialize MQTT
                break;
            case IOT_STATE_MQTT_EVENTS:
                LOG_INF("MQTT events processing...");
                mqtt_process(); // Process MQTT events
                k_msleep(1000);
                break;
            default:
                LOG_ERR("Unknown Wi-Fi state: %d", wifi_state);
                k_msleep(1000);
                break;
        }
    }
}

// -----------------------------------
// Public functions
// -----------------------------------
void iot_thread_start(void)
{
    iot_thread_id = k_thread_create(&iot_thread_data, iot_thread_stack,
                                    K_THREAD_STACK_SIZEOF(iot_thread_stack),
                                    iot_thread_entry, NULL, NULL, NULL,
                                    IOT_THREAD_PRIORITY, 0, K_NO_WAIT);
    printk("IOT thread created\n");
}
