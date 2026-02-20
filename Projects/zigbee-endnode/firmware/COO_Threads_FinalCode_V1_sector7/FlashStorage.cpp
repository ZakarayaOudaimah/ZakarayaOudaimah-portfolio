#include <string.h>
#include "pgmspace.h"
#include <stdint.h>
#include "stm32f4xx_hal.h"
// Flash storage layout:
// Address 0x0807FC00: uint32_t deviceCount (number of MACs stored)
// Address 0x0807FC04: MAC 1 (16 bytes as string + null terminator = 17 bytes)
// Address 0x0807FC15: MAC 2 (next 17 bytes)
// ... and so on up to 10 devices
#include "FlashStorage.h"


#define FLASH_MAC_START 0x0807FC00  // Start of MAC storage (count at this address)
#define FLASH_MAC_DATA 0x0807FC04   // First MAC starts here
#define MAX_MAC_DEVICES 10
#define MAC_STRING_LEN 16    // Length of MAC string (without null)
#define MAC_STORAGE_SIZE 17  // MAC string + null terminator


uint32_t cachedCounter = 0;
struct cached_devices {
  char mac[17];  // Removed = {0} from struct definition
};

cached_devices cachedDevices[MAX_MAC_DEVICES];

// Function to find a device MAC in flash
int Flash_FindDeviceMAC(const char *mac) {
  // To be implemented
  return -1;
}

bool Flash_Write0xFF_Last1KB(void) {
  Serial.println("\n📝 Writing 0xFF to last 1KB...");

  uint32_t startAddr = 0x0807FC00;
  uint32_t endAddr = 0x08080000;  // ✅ CORRECT: up to sector end (exclusive)

  // ✅ CRITICAL: Unlock flash first!
  if (HAL_FLASH_Unlock() != HAL_OK) {
    Serial.println("❌ Failed to unlock flash");
    return false;
  }

  // Clear flags
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

  // Write 0xFF to each byte
  bool success = true;
  uint32_t failCount = 0;

  for (uint32_t addr = startAddr; addr < endAddr; addr++) {
    // Only try to write if not already 0xFF
    if (*(uint8_t *)addr != 0xFF) {
      HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, 0xFF);

      if (status != HAL_OK) {
        success = false;
        failCount++;
        if (failCount <= 5) {
          Serial.print("❌ Failed at 0x");
          Serial.print(addr, HEX);
          Serial.print(": current=0x");
          Serial.print(*(uint8_t *)addr, HEX);
          Serial.print(", status=");
          Serial.println(status);
        }
      }

      HAL_Delay(1);
    }

    // Progress indicator
    if ((addr - startAddr) % 256 == 0) {
      Serial.print(".");
    }
  }

  Serial.println();
  HAL_FLASH_Lock();  // ✅ Lock when done

  if (success) {
    Serial.println("✅ Successfully wrote 0xFF to last 1KB");
  } else {
    Serial.print("❌ Failed at ");
    Serial.print(failCount);
    Serial.println(" locations");
  }

  return success;
}
// Function to load all MACs from flash to RAM and display them
uint8_t Flash_LoadAllMACs(char mac[][17]) {
  Serial.println("\n========== DEVICES IN FLASH ==========");
  Serial.print("📊 Total devices: ");
  Serial.println(cachedCounter);

  if (cachedCounter == 0) {
    Serial.println("No devices found.");
  } else {
    for (int i = 0; i < cachedCounter; i++) {
      Serial.print("Device ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(cachedDevices[i].mac);
      strncpy(mac[i], cachedDevices[i].mac, sizeof(mac[i]));
    }
  }
  Serial.println("=====================================");
  return cachedCounter;
}

// Function to delete a specific MAC from flash
bool Flash_DeleteDeviceMAC(char *mac) {
  // First find the device in cache
  int foundIndex = -1;
  for (int i = 0; i < cachedCounter; i++) {
    if (strcmp(cachedDevices[i].mac, mac) == 0) {
      foundIndex = i;
      break;
    }
  }

  // Check if device exists
  if (foundIndex == -1) {
    Serial.print("❌ MAC not found: ");
    Serial.println(mac);
    return false;
  }

  Serial.print("🗑️ Deleting MAC at index ");
  Serial.print(foundIndex);
  Serial.print(": ");
  Serial.println(mac);

  // Shift all remaining devices left in the cache
  for (int i = foundIndex; i < cachedCounter - 1; i++) {
    strcpy(cachedDevices[i].mac, cachedDevices[i + 1].mac);
  }

  // Clear the last slot
  memset(cachedDevices[cachedCounter - 1].mac, 0, 17);

  // Decrease counter
  uint32_t newCount = cachedCounter - 1;
  cachedCounter = newCount;

  Serial.print("📊 New total devices: ");
  Serial.println(cachedCounter);

  // If no devices left, just erase and write 0 count
  if (cachedCounter == 0) {
    // Unlock flash
    if (HAL_FLASH_Unlock() != HAL_OK) {
      Serial.println("❌ Failed to unlock flash");
      return false;
    }

    // Clear any pending flash flags
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

    // Erase sector 7
    Serial.println("🗑️ Erasing sector 7...");
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = FLASH_SECTOR_7;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {
      Serial.println("❌ Failed to erase sector 7");
      HAL_FLASH_Lock();
      return false;
    }

    HAL_Delay(10);

    // Write 0 count
    Serial.println("💾 Writing count 0...");
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_MAC_START, 0) != HAL_OK) {
      Serial.println("❌ Failed to write count");
      HAL_FLASH_Lock();
      return false;
    }

    HAL_FLASH_Lock();
    Serial.println("✅ All devices deleted, flash cleared");

  } else {
    // We have remaining devices - rewrite all to flash

    // Unlock flash
    if (HAL_FLASH_Unlock() != HAL_OK) {
      Serial.println("❌ Failed to unlock flash");
      return false;
    }

    // Clear any pending flash flags
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

    // Erase sector 7
    Serial.println("🗑️ Erasing sector 7...");
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = FLASH_SECTOR_7;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {
      Serial.println("❌ Failed to erase sector 7");
      HAL_FLASH_Lock();
      return false;
    }

    HAL_Delay(10);

    // Write all remaining MACs from cache to flash
    bool writeSuccess = true;

    for (int i = 0; i < cachedCounter; i++) {
      uint32_t macStorageAddress = FLASH_MAC_DATA + (i * MAC_STORAGE_SIZE);
      char *currentMac = cachedDevices[i].mac;

      Serial.print("💾 Writing MAC ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(currentMac);

      // Write MAC byte by byte (including null terminator)
      for (uint32_t j = 0; j <= MAC_STRING_LEN; j++) {
        uint32_t byteAddress = macStorageAddress + j;
        uint8_t byteToWrite = (j < MAC_STRING_LEN) ? currentMac[j] : 0;

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, byteAddress, byteToWrite) != HAL_OK) {
          Serial.print("❌ Failed to write MAC ");
          Serial.print(i);
          Serial.print(" byte ");
          Serial.println(j);
          writeSuccess = false;
          HAL_FLASH_Lock();
          return false;
        }
        HAL_Delay(1);
      }
    }

    // Write the updated count
    Serial.print("💾 Updating device count to: ");
    Serial.println(cachedCounter);

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_MAC_START, cachedCounter) != HAL_OK) {
      Serial.println("❌ Failed to write device count");
      HAL_FLASH_Lock();
      return false;
    }

    HAL_Delay(5);

    // Verify first MAC (optional)
    if (cachedCounter > 0) {
      char verifyMac[17];
      uint8_t *flashPtr = (uint8_t *)FLASH_MAC_DATA;

      for (int j = 0; j < MAC_STRING_LEN; j++) {
        verifyMac[j] = (char)flashPtr[j];
      }
      verifyMac[MAC_STRING_LEN] = '\0';

      if (strcmp(verifyMac, cachedDevices[0].mac) != 0) {
        Serial.println("❌ First MAC verification failed!");
        Serial.print("  Expected: ");
        Serial.println(cachedDevices[0].mac);
        Serial.print("  Got: ");
        Serial.println(verifyMac);
        writeSuccess = false;
      } else {
        Serial.println("✅ First MAC verified");
      }
    }

    HAL_FLASH_Lock();

    if (writeSuccess) {
      Serial.println("✅ Flash update successful after deletion");
    } else {
      Serial.println("❌ Flash update failed after deletion");
      return false;
    }
  }

  // Show updated list
  Serial.println("\n📋 Updated device list:");
  for (int i = 0; i < cachedCounter; i++) {
    Serial.print("  Device ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(cachedDevices[i].mac);
  }

  return true;
}
// Main function to add a single device MAC to flash
bool Flash_AddDeviceMAC(char *mac) {
  // First check if device already exists in cache/flash
  for (int i = 0; i < cachedCounter; i++) {
    if (strcmp(cachedDevices[i].mac, mac) == 0) {
      Serial.print("ℹ️ MAC already in flash: ");
      Serial.println(mac);
      return true;  // Already exists, consider it success
    }
  }

  // Check if we've reached maximum devices
  if (cachedCounter >= MAX_MAC_DEVICES) {
    Serial.println("❌ Maximum devices (10) reached in flash");
    return false;
  }

  // Calculate MAC string length (should be 16 for valid MAC)
  uint32_t macAddressLen = strlen(mac);
  if (macAddressLen != 16) {
    Serial.print("❌ Invalid MAC length: ");
    Serial.print(macAddressLen);
    Serial.println(" (should be 16)");
    return false;
  }

  // Calculate storage address for new MAC (each MAC takes 17 bytes including null)
  uint32_t macStorageAddress = FLASH_MAC_DATA + (cachedCounter * MAC_STORAGE_SIZE);

  Serial.print("📍 Storing MAC at address: 0x");
  Serial.println(macStorageAddress, HEX);

  // Unlock flash
  if (HAL_FLASH_Unlock() != HAL_OK) {
    Serial.println("❌ Failed to unlock flash");
    return false;
  }

  // Clear any pending flash flags
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

  // IMPORTANT: Erase sector 7 before writing
  Serial.println("🗑️ Erasing sector 7...");
  FLASH_EraseInitTypeDef eraseInit;
  uint32_t sectorError = 0;

  eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  eraseInit.Sector = FLASH_SECTOR_7;
  eraseInit.NbSectors = 1;
  eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {
    Serial.println("❌ Failed to erase sector 7");
    HAL_FLASH_Lock();
    return false;
  }

  HAL_Delay(10);  // Small delay after erase

  HAL_StatusTypeDef status;
  bool writeSuccess = true;

  // First, write ALL existing MACs from cache back to flash
  for (int existingIdx = 0; existingIdx < cachedCounter; existingIdx++) {
    uint32_t existingAddr = FLASH_MAC_DATA + (existingIdx * MAC_STORAGE_SIZE);
    char *existingMac = cachedDevices[existingIdx].mac;

    Serial.print("💾 Rewriting existing MAC ");
    Serial.print(existingIdx);
    Serial.print(": ");
    Serial.println(existingMac);

    for (uint32_t i = 0; i <= MAC_STRING_LEN; i++) {
      uint32_t byteAddress = existingAddr + i;
      uint8_t byteToWrite = (i < MAC_STRING_LEN) ? existingMac[i] : 0;

      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, byteAddress, byteToWrite);

      if (status != HAL_OK) {
        Serial.print("❌ Failed to rewrite existing MAC ");
        Serial.println(existingIdx);
        HAL_FLASH_Lock();
        return false;
      }
      HAL_Delay(1);
    }
  }

  // Then write the new MAC
  Serial.print("💾 Writing new MAC: ");
  Serial.println(mac);

  for (uint32_t i = 0; i <= macAddressLen; i++) {  // <= to write null terminator
    uint32_t byteAddress = macStorageAddress + i;
    uint8_t byteToWrite = (i < macAddressLen) ? mac[i] : 0;  // Last byte is null

    // Program the byte
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, byteAddress, byteToWrite);

    if (status != HAL_OK) {
      Serial.print("❌ Failed to write byte ");
      Serial.print(i);
      Serial.print(" at address 0x");
      Serial.print(byteAddress, HEX);
      Serial.print(". Status: ");
      Serial.println(status);
      writeSuccess = false;
      HAL_FLASH_Lock();
      return false;
    }
    HAL_Delay(1);  // Small delay to ensure programming completes
  }

  // Write the updated count to flash
  uint32_t newCount = cachedCounter + 1;
  Serial.print("💾 Updating device count to: ");
  Serial.println(newCount);

  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_MAC_START, newCount);

  if (status != HAL_OK) {
    Serial.println("❌ Failed to write device count");
    HAL_FLASH_Lock();
    return false;
  }

  HAL_Delay(5);

  // Verify the write by reading back
  char readMac[17];
  uint8_t *flashPtr = (uint8_t *)macStorageAddress;

  // Copy from flash to buffer
  for (uint32_t i = 0; i < macAddressLen; i++) {
    readMac[i] = (char)flashPtr[i];
  }
  readMac[macAddressLen] = '\0';  // Ensure null termination

  Serial.print("📖 Read back from flash: \"");
  Serial.print(readMac);
  Serial.println("\"");

  // Verify the written data
  if (strcmp(mac, readMac) == 0) {
    Serial.println("✅ Write verification successful!");

    // Add to cache only after successful verification
    strncpy(cachedDevices[cachedCounter].mac, mac, MAC_STRING_LEN);
    cachedDevices[cachedCounter].mac[MAC_STRING_LEN] = '\0';
    cachedCounter++;

    Serial.print("➕ Added new MAC to cache: ");
    Serial.println(mac);
    Serial.print("📊 Total devices now: ");
    Serial.println(cachedCounter);
  } else {
    Serial.println("❌ Write verification failed!");
    writeSuccess = false;
  }

  HAL_FLASH_Lock();
  Serial.println("✅ Flash locked");

  return writeSuccess;
}

// Function to initialize flash MAC storage
void Flash_Init(void) {
  // Read the current count from flash
  uint32_t count = *(uint32_t *)FLASH_MAC_START;

  // Check if flash is empty (all 0xFF) or corrupted
  if (count == 0xFFFFFFFF || count > MAX_MAC_DEVICES) {
    // First boot or corrupted - initialize
    Serial.println("🆕 Initializing fresh MAC storage in flash");

    // Need to unlock flash before programming
    if (HAL_FLASH_Unlock() != HAL_OK) {
      Serial.println("❌ Failed to unlock flash");
      return;
    }

    // Clear any pending flash flags
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR);

    // Erase sector 7
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Sector = FLASH_SECTOR_7;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {
      Serial.println("❌ Failed to erase sector 7");
      HAL_FLASH_Lock();
      return;
    }

    HAL_Delay(10);

    // Write 0 to the counter location
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_MAC_START, 0);

    // Lock flash after programming
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
      Serial.print("❌ Failed to initialize count. Status: ");
      Serial.println(status);
      return;
    }

    Serial.println("✅ Flash MAC storage initialized with count = 0");
    cachedCounter = 0;

    // Clear cache
    for (int i = 0; i < MAX_MAC_DEVICES; i++) {
      memset(cachedDevices[i].mac, 0, 17);
    }

  } else {
    // Valid count found in flash - load into cache
    cachedCounter = count;
    Serial.print("\n📊 Number of Nodes Stored in Flash: ");
    Serial.println(cachedCounter);

    // Load MACs into cache
    for (uint32_t i = 0; i < cachedCounter; i++) {
      uint32_t macAddr = FLASH_MAC_DATA + (i * MAC_STORAGE_SIZE);
      uint8_t *flashPtr = (uint8_t *)macAddr;

      for (int j = 0; j < MAC_STRING_LEN; j++) {
        cachedDevices[i].mac[j] = (char)flashPtr[j];
      }
      cachedDevices[i].mac[MAC_STRING_LEN] = '\0';

      Serial.print("  Loaded MAC ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(cachedDevices[i].mac);
    }

    // Clear remaining cache slots
    for (int i = cachedCounter; i < MAX_MAC_DEVICES; i++) {
      memset(cachedDevices[i].mac, 0, 17);
    }
  }
}