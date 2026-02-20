#ifndef ETHERNETHANDLE_H
#define ETHERNETHANDLE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <iostream>
#include <string>








void ConvertStringMACtoCharr(const char *macString);
void ENC_Connect();
void MQTT_ConnectProccessing();
void PublishMACCooRoBroker(char *mess, size_t length);
void PublishDataBase(char *database);
void PublishOnNewNodeAdded(char *MacNewNode, size_t length);
void publishDevices(char *domain, char *mess, int length);
void scaningConnection();
void MQTTLOOP();


#endif