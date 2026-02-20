// FlashStorage.h
#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include <Arduino.h>
#include <STM32FreeRTOS.h>

#define MAX_DEVICES 10
#define MAC_SIZE 17
#define FLASH_QUEUE_SIZE 20

// Core functions
bool Flash_Write0xFF_Last1KB(void);

void Flash_Init(void);
bool Flash_AddDeviceMAC(char *mac);
bool Flash_DeleteDeviceMAC(char *mac);
void Flash_LoadAllMACs(void);
uint8_t Flash_LoadAllMACs(char macs[][17]);

// Debug functions
void Flash_Debug();


#endif