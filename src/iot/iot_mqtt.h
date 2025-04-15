#ifndef IOT_MQTT_H
#define IOT_MQTT_H

void mqtt_process(void);
void mqtt_state_machine();
char* get_gw_addr_ptr();

#endif // IOT_MQTT_H