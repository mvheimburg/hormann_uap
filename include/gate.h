// gate.h
#ifndef GATE_H
#define GATE_H

#define GATE_SPD_DWN 19
#define GATE_SPD_UP 13
#define SWITCH_DELAY 500

#include <PubSubClient.h>
#include <Ethernet.h>
#include "mqtt_specs.h"

enum pin_states { PIN_OPEN, PIN_CLOSED, PIN_BETWEEN };
enum gate_states { GATE_OPEN, GATE_OPENING, GATE_CLOSED, GATE_CLOSING, GATE_STOPPED, GATE_UNCHANGED };
enum light_states { LIGHT_ON, LIGHT_OFF, LIGHT_UNCHANGED };

const char* const gate_state_payloads[] = { PAYLOAD_GATE_OPENED, PAYLOAD_GATE_OPENING, PAYLOAD_GATE_CLOSED, PAYLOAD_GATE_CLOSING };
const char* const light_state_payloads[] = { PAYLOAD_LIGHT_ON, PAYLOAD_LIGHT_OFF };

class Gate {
private:
    // EthernetClient ethClient;
    PubSubClient* client;
    // Scheduler tasker;

    int last_reconnect_attempt = 0;
    int gpio_state_gate_open = 2;
    int gpio_state_gate_closed = 3;
    int gpio_state_light_on = 4;
    int gpio_command_gate_open = A0;
    int gpio_command_gate_close = A1;
    int gpio_command_gate_stop = A2;
    int gpio_command_light_on = A3;
    const char* last_command = NULL;
    long last_move_t;
    

  public:
    Gate(PubSubClient* client);
    void checkState();
    void publishState();
    void publishPos();

    void checkLightState();
    void publishLightState();

    pin_states pinState();
    void triggerGate(int pin);
    void openGate();
    void closeGate(); 
    void stopGate();
    void triggerLight(bool on);
    void activate(char* topic, byte* payload, unsigned int length);
    void subscribe();

    bool moving = false;
    int position = 0;
    gate_states state = GATE_UNCHANGED;
    light_states light = LIGHT_UNCHANGED;



};

#endif