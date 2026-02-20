#ifndef JSONHANDLE_H
#define JSONHANDLE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <ArduinoJson.h>


struct JsonData {
  char mac[32];
  char command[32];
  char userId[64];
  char transactionId[64];
  bool validMac;
};


bool checkJSONFormat(char *JSONmessage);
JsonData parseJson(const char *msg);

#endif