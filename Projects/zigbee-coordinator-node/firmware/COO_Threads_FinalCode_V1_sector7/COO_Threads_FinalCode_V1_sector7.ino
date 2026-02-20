#include <STM32FreeRTOS.h>
#include <ZigBeeHandle.h>
#include <EthernetHandle.h>
#include <iostream>
#include <string>
#include <ArduinoJson.h>
#include "JSONHandle.h"
#include "FlashStorage.h"
// #include <EEPROM.h>


extern bool ZigBee_init;
extern uint8_t deviceCount;
extern bool DHCP_FlageOK;
extern bool MQTT_FlageOK;
extern bool SendLogMACCooToBroker;
extern bool sendDatabase;
extern uint8_t belongNodesNumber;

#define BOARD_LED_PIN PC13
#define LED_PIN PB0

//#include <SoftwareSerial.h>
//SoftwareSerial mySerial(PA3, PA2); // RX, TX
HardwareSerial Serial2(PA3, PA2);  // RX, TX

extern char MAC_Address[17];
extern bool getByteMAC;


static void task1(void *pvParameters) {
  for (;;) {
    indicators();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static void task2(void *pvParameters) {
  for (;;) {
    if (ZigBee_init == false) {
      initZigBee();
      vTaskDelay(100);
    } else if (ZigBee_init == true) {
      HandlEveryIncoming();
      handleTopics();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}


static void task3(void *pvParameters) {
  for (;;) {
    TimersForNodes();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static void task4(void *pvParameters) {
  for (;;) {
    //   // STEP 1: Initialize Ethernet first
    if (ZigBee_init == true && !DHCP_FlageOK && !MQTT_FlageOK) {
      ENC_Connect();
    }

    // STEP 2: After DHCP is OK, connect MQTT
    else if (DHCP_FlageOK && !MQTT_FlageOK) {
      MQTT_ConnectProccessing();

      // Don't try to publish until MQTT is connected!
      if (belongNodesNumber == 0) {
        SendLogMACCooToBroker = false;
      }
    }

    // STEP 3: Only after MQTT is connected, do MQTT operations
    else if (MQTT_FlageOK) {

      if (!SendLogMACCooToBroker) {
        make_string_of_belong_nodes_and_send_them();
        vTaskDelay(pdMS_TO_TICKS(200));
      }

      else if (!sendDatabase && SendLogMACCooToBroker) {
        sendDataBASE_AfterConnectionIsDone();
        send_saved_messages_when_net_is_on();
      }

      else if (sendDatabase && SendLogMACCooToBroker) {
        SendNewMacEndDevice();
      }

      // Always run MQTT loop when connected
      MQTTLOOP();
    }

    // Always check connection status
    scaningConnection();
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms delay
  }
}


// static void task_test_eeprom(void *pvParameters) {
//   // Wait a bit for everything to initialize
//   vTaskDelay(pdMS_TO_TICKS(2000));

//   Serial.println("\n\n========== EEPROM Adding Test Devices ==========");

//   requestEEPROMAdd("00158d00099ab35a");
//   vTaskDelay(pdMS_TO_TICKS(100));  // Small delay between requests

//   requestEEPROMAdd("00158d00092ee686");
//   vTaskDelay(pdMS_TO_TICKS(100));

//   requestEEPROMAdd("00158d00092ee685");
//   vTaskDelay(pdMS_TO_TICKS(100));

//   Serial.println("\n\n========== Test Devices Queued ==========");

//   // Give the processor task time to actually write to EEPROM
//   vTaskDelay(pdMS_TO_TICKS(500));

//   // NOW check what was actually written
//   Serial.println("\n\n========== VERIFYING EEPROM AFTER WRITES ==========");
//   Flash_Debug();
//   load_belong_devices_from_EEPROM();

//   // Delete this task after adding devices
//   vTaskDelete(NULL);
// }

// In COO_Threads_FinalCode_V1_with_flashSave.ino
// Replace EEPROM initialization with Flash initialization:



void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(57600);
  delay(2000);

  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(PB1, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Flash_Init();

  // Flash_AddDeviceMAC("00158d00092ee685");
  // Flash_AddDeviceMAC("00158d00092ee686");
  // Flash_AddDeviceMAC("00158d00099ab35a");


  // Flash_LoadAllMACs();
  // // char macs[MAX_DEVICES][17];
  // // uint8_t d = Flash_LoadAllMACs(macs);

  // Flash_DeleteDeviceMAC("00158d00092ee686");
  // Flash_LoadAllMACs();
  // // d = Flash_LoadAllMACs(macs);

  // Flash_DeleteDeviceMAC("00158d00092ee685");
  // Flash_LoadAllMACs();
  // // d = Flash_LoadAllMACs(macs);


  // Flash_DeleteDeviceMAC("00158d00099ab35a");
  // Flash_LoadAllMACs();
  // // d = Flash_LoadAllMACs(macs);

  load_belong_devices_from_EEPROM();


  Serial.println("\n\n========== CREATING TASKS ==========");

  xTaskCreate(task1, "Task1",
              configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(task2, "Task2", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(task3, "Task3", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(task4, "Task4", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

  vTaskStartScheduler();
}
void loop() {
}
/*

  // MAC address string to write
  char macAddress[] = "00158d00092ee685";       // 16 chars + null terminator
  uint32_t macAddressLen = sizeof(macAddress);  // 17 bytes (including null)

  // Calculate required space - we need 17 bytes, which fits in 5 words (20 bytes)
  // Address for MAC storage (using sector 7, after the test data)
  uint32_t macStorageAddress = 0x0807FC10;  // 16 bytes after test data (0x0807FC00 + 16)

  Serial.println("\n\n========== MAC ADDRESS FLASH TEST ==========");

  // Step 1: Unlock flash
  Serial.println("\n📋 Step 1: Unlocking flash");
  if (HAL_FLASH_Unlock() == HAL_OK) {
    Serial.println("✅ Flash unlocked successfully");
  } else {
    Serial.println("❌ Failed to unlock flash");
    return;
  }

  // Step 2: Clear flash flags
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
  Serial.println("✅ Flash flags cleared");

  // Step 3: Erase the sector containing our MAC address
  // Note: This will erase everything in sector 7 including the test data from earlier
  Serial.println("\n📋 Step 2: Erasing sector 7 for MAC storage");
  FLASH_EraseInitTypeDef eraseInit;
  uint32_t sectorError = 0;

  eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  eraseInit.Sector = FLASH_SECTOR_7;
  eraseInit.NbSectors = 1;
  eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) == HAL_OK) {
    Serial.println("✅ Sector 7 erased successfully");
  } else {
    Serial.println("❌ Failed to erase sector 7");
    HAL_FLASH_Lock();
    return;
  }

  // Step 4: Write MAC address string to flash
  Serial.println("\n📋 Step 3: Writing MAC address to flash");
  Serial.print("MAC Address to store: \"");
  Serial.print(macAddress);
  Serial.println("\"");

  HAL_StatusTypeDef status;
  bool writeSuccess = true;

  // Write the MAC address byte by byte (as 8-bit values)
  for (uint32_t i = 0; i < macAddressLen; i++) {
    // Calculate address for this byte
    uint32_t byteAddress = macStorageAddress + i;

    // Program the byte
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, byteAddress, macAddress[i]);

    if (status != HAL_OK) {
      Serial.print("❌ Failed to write byte ");
      Serial.print(i);
      Serial.print(" at address 0x");
      Serial.print(byteAddress, HEX);
      Serial.print(". Status: ");
      Serial.println(status);
      writeSuccess = false;
      break;
    }

    // Small delay to ensure programming completes
    HAL_Delay(1);
  }

  if (writeSuccess) {
    Serial.print("✅ Successfully wrote ");
    Serial.print(macAddressLen);
    Serial.println(" bytes to flash");
  }

  // Step 5: Read back and verify MAC address
  Serial.println("\n📋 Step 4: Reading MAC address from flash");

  char readMac[20];  // Buffer for read MAC (larger than needed)
  uint8_t *flashPtr = (uint8_t *)macStorageAddress;

  // Copy from flash to buffer
  for (uint32_t i = 0; i < macAddressLen; i++) {
    readMac[i] = (char)flashPtr[i];
  }

  Serial.print("📖 Read from flash: \"");
  Serial.print(readMac);
  Serial.println("\"");

  // Verify
  if (strcmp(macAddress, readMac) == 0) {
    Serial.println("✅ MAC address verification successful!");
  } else {
    Serial.println("❌ MAC address verification failed!");
    Serial.print("Expected: \"");
    Serial.print(macAddress);
    Serial.print("\", Got: \"");
    Serial.print(readMac);
    Serial.println("\"");
  }

  // Step 6: Display raw hex values
  Serial.println("\n📋 Raw hex values at MAC storage location:");
  for (int i = 0; i < 20; i++) {  // Show 20 bytes to see padding if any
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(" (0x");
    Serial.print(macStorageAddress + i, HEX);
    Serial.print("): 0x");

    uint8_t val = flashPtr[i];
    if (val < 0x10) Serial.print("0");
    Serial.print(val, HEX);

    // Print ASCII if printable
    if (val >= 32 && val <= 126) {
      Serial.print(" '");
      Serial.print((char)val);
      Serial.print("'");
    }
    Serial.println();
  }

  // Step 7: Alternative - Store as binary (more efficient)
  Serial.println("\n📋 Step 5: Alternative - Store MAC as binary (6 bytes)");

  // Convert hex string to binary (6 bytes)
  uint8_t binaryMac[6];
  uint32_t binaryStorageAddress = 0x0807FC30;  // Different location

  // Parse hex string to binary
  for (int i = 0; i < 6; i++) {
    char byteStr[3] = { macAddress[i * 2], macAddress[i * 2 + 1], 0 };
    binaryMac[i] = (uint8_t)strtol(byteStr, NULL, 16);
  }

  // Write binary MAC
  Serial.print("Binary MAC to store: ");
  for (int i = 0; i < 6; i++) {
    Serial.print("0x");
    if (binaryMac[i] < 0x10) Serial.print("0");
    Serial.print(binaryMac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  writeSuccess = true;
  for (int i = 0; i < 6; i++) {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, binaryStorageAddress + i, binaryMac[i]);
    if (status != HAL_OK) {
      Serial.print("❌ Failed to write binary byte ");
      Serial.println(i);
      writeSuccess = false;
      break;
    }
    HAL_Delay(1);
  }

  if (writeSuccess) {
    Serial.println("✅ Binary MAC written successfully");

    // Read back binary MAC
    uint8_t readBinaryMac[6];
    for (int i = 0; i < 6; i++) {
      readBinaryMac[i] = *(uint8_t *)(binaryStorageAddress + i);
    }

    Serial.print("📖 Read binary MAC: ");
    for (int i = 0; i < 6; i++) {
      Serial.print("0x");
      if (readBinaryMac[i] < 0x10) Serial.print("0");
      Serial.print(readBinaryMac[i], HEX);
      if (i < 5) Serial.print(":");
    }
    Serial.println();

    // Verify binary MAC
    if (memcmp(binaryMac, readBinaryMac, 6) == 0) {
      Serial.println("✅ Binary MAC verification successful!");
    } else {
      Serial.println("❌ Binary MAC verification failed!");
    }
  }

  // Step 8: Lock flash
  HAL_FLASH_Lock();
  Serial.println("\n✅ Flash locked");

  Serial.println("\n========== MAC ADDRESS TEST COMPLETE ==========");

*/

/*
Serial.println("\n\n========== DIRECT HAL FLASH TEST ==========");

  // Test data
  uint32_t testAddress = 0x0807FC00;  // Sector 7 start
  uint32_t testData = 0x12345678;
  uint32_t readData;

  // Step 1: Unlock flash
  Serial.println("\n📋 Step 1: Unlocking flash");
  if (HAL_FLASH_Unlock() == HAL_OK) {
    Serial.println("✅ Flash unlocked successfully");
  } else {
    Serial.println("❌ Failed to unlock flash");
    return;
  }

  // Step 2: Clear flash flags
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
  Serial.println("✅ Flash flags cleared");

  // Step 3: Erase sector 7
  Serial.println("\n📋 Step 2: Erasing sector 7");
  FLASH_EraseInitTypeDef eraseInit;
  uint32_t sectorError = 0;

  eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  eraseInit.Sector = FLASH_SECTOR_7;
  eraseInit.NbSectors = 1;
  eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) == HAL_OK) {
    Serial.println("✅ Sector 7 erased successfully");
  } else {
    Serial.println("❌ Failed to erase sector 7");
    HAL_FLASH_Lock();
    return;
  }

  // Step 4: Read after erase (should be 0xFFFFFFFF)
  // IMPORTANT: Cast to uint32_t pointer to read 32-bit value
  readData = *(uint32_t *)testAddress;
  Serial.print("📖 Read after erase: 0x");
  Serial.println(readData, HEX);
  if (readData == 0xFFFFFFFF) {
    Serial.println("✅ Erase verified (all FF)");
  } else {
    Serial.println("❌ Erase verification failed");
    Serial.println("⚠️ This suggests the sector might not be properly erased or address is wrong");
  }

  // Step 5: Write test data
  Serial.println("\n📋 Step 3: Writing test data");
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, testAddress, testData);

  if (status == HAL_OK) {
    Serial.print("✅ Successfully wrote 0x");
    Serial.print(testData, HEX);
    Serial.print(" to address 0x");
    Serial.println(testAddress, HEX);
  } else {
    Serial.print("❌ Failed to write. Status: ");
    Serial.println(status);
    HAL_FLASH_Lock();
    return;
  }

  // Step 6: Read back and verify (as 32-bit)
  readData = *(uint32_t *)testAddress;
  Serial.print("📖 Read back (32-bit): 0x");
  Serial.println(readData, HEX);

  if (readData == testData) {
    Serial.println("✅ Write verified successfully!");
  } else {
    Serial.println("❌ Write verification failed!");
    Serial.print("Expected: 0x");
    Serial.print(testData, HEX);
    Serial.print(" Got: 0x");
    Serial.println(readData, HEX);
  }

  // Step 7: Write second word
  Serial.println("\n📋 Step 4: Writing second word");
  uint32_t testAddress2 = 0x0807FC04;  // Next word (4 bytes later)
  uint32_t testData2 = 0x9ABCDEF0;

  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, testAddress2, testData2);

  if (status == HAL_OK) {
    Serial.print("✅ Successfully wrote 0x");
    Serial.print(testData2, HEX);
    Serial.print(" to address 0x");
    Serial.println(testAddress2, HEX);
  } else {
    Serial.print("❌ Failed to write second word. Status: ");
    Serial.println(status);
  }

  // Step 8: Read back second word (as 32-bit)
  readData = *(uint32_t *)testAddress2;
  Serial.print("📖 Read back second word (32-bit): 0x");
  Serial.println(readData, HEX);

  if (readData == testData2) {
    Serial.println("✅ Second write verified!");
  } else {
    Serial.println("❌ Second write verification failed!");
    Serial.print("Expected: 0x");
    Serial.print(testData2, HEX);
    Serial.print(" Got: 0x");
    Serial.println(readData, HEX);
  }

  // Step 9: Read both words as 32-bit
  Serial.println("\n📋 Step 5: Final sector 7 contents (32-bit reads)");
  Serial.print("Address 0x0807FC00: 0x");
  Serial.println(*(uint32_t *)0x0807FC00, HEX);
  Serial.print("Address 0x0807FC04: 0x");
  Serial.println(*(uint32_t *)0x0807FC04, HEX);
  Serial.print("Address 0x0807FC08: 0x");
  Serial.println(*(uint32_t *)0x0807FC08, HEX);

  // Step 10: Show raw bytes at address 0x0800C000
  Serial.println("\n📋 Raw bytes at 0x0807FC00:");
  uint8_t *bytePtr = (uint8_t *)0x0807FC00;
  for (int i = 0; i < 16; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": 0x");
    Serial.println(bytePtr[i], HEX);
  }

  // Step 11: Lock flash
  HAL_FLASH_Lock();
  Serial.println("\n✅ Flash locked");

  Serial.println("\n========== TEST COMPLETE ==========");

*/
// Serial.println("\n\n========== DIRECT HAL FLASH TEST ==========");

//   // Test data
//   uint32_t testAddress = 0x08060000;  // Sector 7 start
//   uint32_t testData = 0x12345678;
//   uint32_t readData;

//   // Step 1: Unlock flash
//   Serial.println("\n📋 Step 1: Unlocking flash");
//   if (HAL_FLASH_Unlock() == HAL_OK) {
//     Serial.println("✅ Flash unlocked successfully");
//   } else {
//     Serial.println("❌ Failed to unlock flash");
//     return;
//   }

//   // Step 2: Clear flash flags
//   __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);
//   Serial.println("✅ Flash flags cleared");

//   // Step 3: Erase sector 7
//   Serial.println("\n📋 Step 2: Erasing sector 7");
//   FLASH_EraseInitTypeDef eraseInit;
//   uint32_t sectorError = 0;

//   eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
//   eraseInit.Sector = FLASH_SECTOR_7;
//   eraseInit.NbSectors = 1;
//   eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

//   if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) == HAL_OK) {
//     Serial.println("✅ Sector 7 erased successfully");
//   } else {
//     Serial.println("❌ Failed to erase sector 7");
//     HAL_FLASH_Lock();
//     return;
//   }

//   // Step 4: Read after erase (should be 0xFFFFFFFF)
//   // IMPORTANT: Cast to uint32_t pointer to read 32-bit value
//   readData = *(uint32_t *)testAddress;
//   Serial.print("📖 Read after erase: 0x");
//   Serial.println(readData, HEX);
//   if (readData == 0xFFFFFFFF) {
//     Serial.println("✅ Erase verified (all FF)");
//   } else {
//     Serial.println("❌ Erase verification failed");
//     Serial.println("⚠️ This suggests the sector might not be properly erased or address is wrong");
//   }

//   // Step 5: Write test data
//   Serial.println("\n📋 Step 3: Writing test data");
//   HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, testAddress, testData);

//   if (status == HAL_OK) {
//     Serial.print("✅ Successfully wrote 0x");
//     Serial.print(testData, HEX);
//     Serial.print(" to address 0x");
//     Serial.println(testAddress, HEX);
//   } else {
//     Serial.print("❌ Failed to write. Status: ");
//     Serial.println(status);
//     HAL_FLASH_Lock();
//     return;
//   }

//   // Step 6: Read back and verify (as 32-bit)
//   readData = *(uint32_t *)testAddress;
//   Serial.print("📖 Read back (32-bit): 0x");
//   Serial.println(readData, HEX);

//   if (readData == testData) {
//     Serial.println("✅ Write verified successfully!");
//   } else {
//     Serial.println("❌ Write verification failed!");
//     Serial.print("Expected: 0x");
//     Serial.print(testData, HEX);
//     Serial.print(" Got: 0x");
//     Serial.println(readData, HEX);
//   }

//   // Step 7: Write second word
//   Serial.println("\n📋 Step 4: Writing second word");
//   uint32_t testAddress2 = 0x08060004;  // Next word (4 bytes later)
//   uint32_t testData2 = 0x9ABCDEF0;

//   status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, testAddress2, testData2);

//   if (status == HAL_OK) {
//     Serial.print("✅ Successfully wrote 0x");
//     Serial.print(testData2, HEX);
//     Serial.print(" to address 0x");
//     Serial.println(testAddress2, HEX);
//   } else {
//     Serial.print("❌ Failed to write second word. Status: ");
//     Serial.println(status);
//   }

//   // Step 8: Read back second word (as 32-bit)
//   readData = *(uint32_t *)testAddress2;
//   Serial.print("📖 Read back second word (32-bit): 0x");
//   Serial.println(readData, HEX);

//   if (readData == testData2) {
//     Serial.println("✅ Second write verified!");
//   } else {
//     Serial.println("❌ Second write verification failed!");
//     Serial.print("Expected: 0x");
//     Serial.print(testData2, HEX);
//     Serial.print(" Got: 0x");
//     Serial.println(readData, HEX);
//   }

//   // Step 9: Read both words as 32-bit
//   Serial.println("\n📋 Step 5: Final sector 7 contents (32-bit reads)");
//   Serial.print("Address 0x0800C000: 0x");
//   Serial.println(*(uint32_t *)0x0800C000, HEX);
//   Serial.print("Address 0x0800C004: 0x");
//   Serial.println(*(uint32_t *)0x0800C004, HEX);
//   Serial.print("Address 0x0800C008: 0x");
//   Serial.println(*(uint32_t *)0x0800C008, HEX);

//   // Step 10: Show raw bytes at address 0x0800C000
//   Serial.println("\n📋 Raw bytes at 0x0800C000:");
//   uint8_t *bytePtr = (uint8_t *)0x0800C000;
//   for (int i = 0; i < 16; i++) {
//     Serial.print("Byte ");
//     Serial.print(i);
//     Serial.print(": 0x");
//     Serial.println(bytePtr[i], HEX);
//   }

//   // Step 11: Lock flash
//   HAL_FLASH_Lock();
//   Serial.println("\n✅ Flash locked");

//   Serial.println("\n========== TEST COMPLETE ==========");



// void testEEPROM() {
//   Serial.println("\n\n========== EEPROM TEST EXAMPLE ==========");

//   // 1. Initialize and clear EEPROM
//   Flash_Init();
//   Flash_ClearAll();

//   // 2. Add devices one by one (2-3ms each)
//   Serial.println("\n--- Adding devices ---");
//   Flash_AddDeviceMAC("00158d00099ab35a");
//   Flash_AddDeviceMAC("00158d00092ee686");
//   Flash_AddDeviceMAC("00158d00092ee685");
//   Flash_AddDeviceMAC("00158d00099ab555");

//   // 3. Verify each device was written correctly
//   Serial.println("\n--- Verification ---");
//   Flash_VerifyMAC("00158d00099ab35a", 0);
//   Flash_VerifyMAC("00158d00092ee686", 1);
//   Flash_VerifyMAC("00158d00092ee685", 2);
//   Flash_VerifyMAC("00158d00099ab555", 3);

//   // 4. Load and display all devices
//   char macs[MAX_DEVICES][17];
//   uint8_t count = Flash_LoadAllDevices(macs);

//   Serial.println("\n--- Current devices in EEPROM ---");
//   for (uint8_t i = 0; i < count; i++) {
//     Serial.print(i);
//     Serial.print(": ");
//     Serial.println(macs[i]);
//   }

//   // 5. Remove a device (middle)
//   Serial.println("\n--- Removing device 00158d00092ee686 ---");
//   Flash_RemoveDeviceMAC("00158d00092ee686");

//   // 6. Load and display after removal
//   count = Flash_LoadAllDevices(macs);

//   Serial.println("\n--- Devices after removal ---");
//   for (uint8_t i = 0; i < count; i++) {
//     Serial.print(i);
//     Serial.print(": ");
//     Serial.println(macs[i]);
//   }

//   // 7. Remove last device
//   Serial.println("\n--- Removing last device 00158d00099ab555 ---");
//   Flash_RemoveDeviceMAC("00158d00099ab555");

//   // 8. Load and display final state
//   count = Flash_LoadAllDevices(macs);

//   Serial.println("\n--- Final devices ---");
//   for (uint8_t i = 0; i < count; i++) {
//     Serial.print(i);
//     Serial.print(": ");
//     Serial.println(macs[i]);
//   }

//   // 9. Show debug info
//   Flash_Debug();

//   Serial.println("\n========== TEST COMPLETE ==========");
//   // Load and print all devices
// }