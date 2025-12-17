#include <stdint.h>
#ifndef ZIGBEEHANDLE_H
#define ZIGBEEHANDLE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>




#define MAX_DEVICES 10

void initZigBee();
bool hardwareCheck(char *Mess);
void MAC_ZigBee(char *Mess);
void CommToZigBee(char *Mess , bool checkCooNet = false , int Timing = 0);
void HandlEveryIncoming();
void handleOpenDoor();
//void runningEveryTimeUntilAlertOn();
//void deletedDevicesProccessing();
//void commandReplyFailure(char *mac , uint8_t indexOfDevice);
//void runningEveryTimeUntilFailureOn();

void smartLockMessage(char *mac , uint8_t indexOfNode = 0 ,  bool disconnected = false , bool openDoorFailure = false  , bool batteryCheck = false , bool openDoorSuccess = false);
void TimersForNodes();
//void UpdateNetwork();
void sendDataBASE_AfterConnectionIsDone();
void SendNewMacEndDevice();
void indicators();


#endif