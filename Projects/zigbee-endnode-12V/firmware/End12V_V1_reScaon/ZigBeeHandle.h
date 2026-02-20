#ifndef ZIGBEEHANDLE_H
#define ZIGBEEHANDLE_H

#include <Arduino.h>

void initZigBee();
void MAC_ZigBee(char *Mess);
void CommToZigBee(char *Mess , bool isConnected);
bool reconnecting_belongCoo();
void Checking();
// bool isConnectedToCoordinator(const char *response);
void send_log_message();
void HandleMessages();
void SleepWithRecieve();
#endif