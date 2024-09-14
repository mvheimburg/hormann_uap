// mqtt_specs.h
#ifndef MQTT_SPECS_H
#define MQTT_SPECS_H

#define MQTT_GATE_TOPIC_CMD "garage/gate/cmd"
#define PAYLOAD_CMD_GATE_OPEN "OPEN"
#define PAYLOAD_CMD_GATE_CLOSE "CLOSE"
#define PAYLOAD_CMD_GATE_STOP "STOP"

// MQTT Gate state feedback, topic and paylaods
#define MQTT_GATE_TOPIC_STATUS "garage/gate/state"
#define PAYLOAD_GATE_OPENED "open"
#define PAYLOAD_GATE_OPENING "opening"
#define PAYLOAD_GATE_CLOSED "closed"
#define PAYLOAD_GATE_CLOSING "closing"


#define MQTT_GATE_TOPIC_POSITION "garage/gate/position"
#define GATE_POS_OPEN 100
#define GATE_POS_CLOSED 0

// MQTT Light commands, topic and paylaods
#define MQTT_LIGHT_TOPIC_CMD "garage/light/cmd"
#define PAYLOAD_CMD_LIGHT_ON "ON"
#define PAYLOAD_CMD_LIGHT_OFF "OFF"

// MQTT Light state feedback, topic and paylaods
#define MQTT_LIGHT_TOPIC_STATUS "garage/light/state"
#define PAYLOAD_LIGHT_ON "on"
#define PAYLOAD_LIGHT_OFF "off"



#endif