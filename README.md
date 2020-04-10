Garage door controller for Hormann UAP1 module.
Developed for Keystudio W5500 to interact with 
Home Asssitant over MQTT.

Inspired by https://github.com/hpd96/garagentor-esp8266


OUTPUTS:
Connect Arduino PIN 08 to S3 of the UAP1 - Open gate
Connect Arduino PIN 09 to S4 of the UAP1 - Close gate
Connect Arduino PIN 11 to S5 of the UAP1 - Light on/off


Connect Arduino PIN 10 to S2 of the UAP1 - Partially open gate // Not in use with current Home Assistant implementation. Might be supported in future.

INPUTS:
Connect Arduino PIN 02 to 01.8 of the UAP1 - Gate open
Connect Arduino PIN 03 to 02.8 of the UAP1 - Gate closed
Connect Arduino PIN 04 to 03.8 of the UAP1 - Light on/off

