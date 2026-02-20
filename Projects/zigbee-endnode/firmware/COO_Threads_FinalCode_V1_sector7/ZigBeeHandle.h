#include <stdint.h>
#ifndef ZIGBEEHANDLE_H
#define ZIGBEEHANDLE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>




#define MAX_DEVICES 10

void initZigBee();
bool hardwareCheck(char *Mess);
void MAC_ZigBee(char *Mess);
void CommToZigBee(char *Mess, bool checkCooNet = false, int Timing = 0);
void HandlEveryIncoming();
void handleTopics();

void publish_on_topics(char *mac, uint8_t indexOfNode, bool disconnected, bool openDoorFailure, bool passwordnotset, bool batteryCheckFailed, bool batteryCheck, bool openDoorSuccess, bool passwordset);

void TimersForNodes();
void sendDataBASE_AfterConnectionIsDone();
void SendNewMacEndDevice();
void send_saved_messages_when_net_is_on();
void indicators();

// void scannigEEPROM();
void make_string_of_belong_nodes_and_send_them();
void load_belong_devices_from_EEPROM();


#endif