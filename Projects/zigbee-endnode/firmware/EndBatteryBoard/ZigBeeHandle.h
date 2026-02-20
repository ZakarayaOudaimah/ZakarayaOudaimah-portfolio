#ifndef ZIGBEEHANDLE_H
#define ZIGBEEHANDLE_H

#include <Arduino.h>

void initZigBee();
void MAC_ZigBee(char *Mess);
void CommToZigBee(char *Mess, bool isConnected);
bool reconnecting_belongCoo();
void Checking();
void send_log_message();
void HandleMessages();
void SleepWithRecieve();
void Keypad_get_password();
void handle_lines(char *mess);

#endif