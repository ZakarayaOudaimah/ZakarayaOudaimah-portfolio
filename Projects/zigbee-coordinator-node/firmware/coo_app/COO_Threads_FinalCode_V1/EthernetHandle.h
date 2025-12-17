#ifndef ETHERNETHANDLE_H
#define ETHERNETHANDLE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <iostream>
#include <string>








void ConvertStringMACtoCharr(const char *macString);
void ENC_Connect();
void MQTT_ConnectProccessing();
void PublishMACCooRoBroker();
void PublishDataBase(char *database);
void PublishOnNewNodeAdded(char *MacNewNode);
void PublishUpdateOnNetwork(char *UpdateMessga);
//void PublishCommandReply(char *CommandReplyMessga);
//void PublishAlertMessages(char *AlertMessga);
void publishDevices(char *domain , char *mess);
void scaningConnection();
void MQTTLOOP();


#endif