#include "pgmspace.h"
#include <ZigBeeHandle.h>

#include <sys/_stdint.h>
#include "WSerial.h"
#include <stdint.h>
#include "wiring_time.h"
#include "SoftwareSerial.h"
#include <string.h>


#include <EthernetHandle.h>
#include "JSONHandle.h"
#include <ArduinoJson.h>

#include "FlashStorage.h"

//extern SoftwareSerial Serial2;

TickType_t lastReceiveTime = 0;
TickType_t timingfordeletedScaningDevices = 0;

/*struct alertMessages {
  char messagesOfAlerts[256];
};

struct FailedtMessages {
  char messagesOfFailures[256];
};*/

struct device_types {
  char smartlocks[11] = "smartlocks";
};

struct eventType {
  char success[8] = "success";
  char failure[8] = "failure";
  char lost[5] = "lost";
};

eventType eventDevice;
device_types types;


struct DeviceMessage {
  char mac[17] = { 0 };
  char message[128] = { 0 };
  char payload[512];
  TickType_t TimerNode = 0;
  bool sendMAcEndDeviceToBroker = false;
  //bool isNotBelongToThisCoo = false;
  char transactionId[32] = { 0 };
  char userId[32] = { 0 };
  char propertyId[32] = { 0 };
  char name[32] = { 0 };
  char device_type[32] = { 0 };
  char batterlevel[7] = { 0 };
  bool sleepAtr = false;
  bool waitingReceiveOKfromNodewithoutSleep = false;
};

struct belongNodesToOurCoo {
  char mac[17] = { 0 };
  bool added_to_EEPROM = false;
};

struct NotbelongNodesToOurCoo {
  char mac[17] = { 0 };
  bool sendTheresponseToNodeThatNotbelong = false;
};

struct messages_to_save {
  char message_saved_duto_internet_off[256] = { 0 };
};

uint32_t timeout = 32000;
uint8_t deviceCount = 0;
uint8_t belongNodesNumber = 0;
uint8_t notbelongNodesNumber = 0;
uint8_t counterformessagessaved = 0;
//uint8_t numDisConnedted = 0;//

DeviceMessage devices[MAX_DEVICES];
belongNodesToOurCoo belongEndDevices[MAX_DEVICES];
NotbelongNodesToOurCoo notbelongEndDevices[MAX_DEVICES];
messages_to_save messages_saved[MAX_DEVICES + 20];
//deletedDevices OutDevices[MAX_DEVICES];
//alertMessages alerts[MAX_DEVICES];
//FailedtMessages Failures[MAX_DEVICES];

char buffer[512] = { 0 };

char response[512];
int counter_response;
bool ZigBee_init = false;
char MAC_Address[17];
extern char messageArriverOnMACCooTopic[512];
extern bool publicAlertReplayFlage;
bool getMAC;
bool hardwareZigBee = false;
bool aNodeDiscoonected = false;
bool checkCooNetworkSetting = false;
//bool deletedNodeImmediatily = false;
//bool ThereisAnUpdate = false;

/*
extern char server[];
#define MQTT_PORT  8883
extern const int rand_pin;
extern EthernetClient base_client;
extern SSLClient client(base_client, TAs, (size_t)TAs_NUM, rand_pin);

extern void MQTTcallback(char* topic, byte* payload, unsigned int length);
extern PubSubClient MQTT(server,MQTT_PORT,MQTTcallback,client);*/
extern bool MQTT_FlageOK;
extern bool DHCP_FlageOK;
bool deletedDevicesExsite = false;
bool alertCommand = false;
bool failureCommand = false;
bool messageArrivedOnBleongsNodes = false;
bool messageArrivedOnDeletingOnBleongsNodes = false;
bool commandTome = false;
//extern bool messageArrivedOnTopic;
bool sendDatabase = false;



//hardware Check
bool hardwareCheck(char *Mess) {
  if (strlen(Mess) > 0) {
    if (strncmp(Mess, "Error", 5) == 0 || strncmp(Mess, "Enter AT Mode", 13) == 0) {
      return true;
    } else {
      Serial.println("Hardware Not detected, please see you hardware....");
      return false;
    }
  } else {
    Serial.println("Hardware Not detected, please see you hardware....");
    return false;
  }
}


bool isHexString(const char *str, int len) {
  for (int i = 0; i < len; i++) {
    if (!isxdigit((unsigned char)str[i])) return false;
  }
  return true;
}

bool isAllZeros(const char *str, int len) {
  for (int i = 0; i < len; i++) {
    if (str[i] != '0') return false;
  }
  return true;
}

bool checkCooNetwork(char *Mess) {

  char *p_pan = strstr(Mess, "PANID:");
  char *p_expan = strstr(Mess, "EXPANID:");

  if (p_pan == NULL || p_expan == NULL) {
    Serial.println("\nPANID or EXPANID not found in the message!");
    return false;
  }

  /* Move pointer to after "PANID: " */
  p_pan += strlen("PANID: ");

  /* Skip optional "0x" */
  if (p_pan[0] == '0' && (p_pan[1] == 'x' || p_pan[1] == 'X'))
    p_pan += 2;

  char panid[5] = { 0 };
  strncpy(panid, p_pan, 4);

  /* Move pointer to after "EXPANID: " */
  p_expan += strlen("EXPANID: ");

  if (p_expan[0] == '0' && (p_expan[1] == 'x' || p_expan[1] == 'X'))
    p_expan += 2;

  char expanid[17] = { 0 };
  strncpy(expanid, p_expan, 16);

  /* Validate PANID */
  if (!isHexString(panid, 4) || isAllZeros(panid, 4)) {
    Serial.printf("\nInvalid PANID: %s\n", panid);
    return false;
  } else {
    Serial.printf("\nValid PANID: %s\n", panid);
  }

  /* Validate EXPANID */
  if (!isHexString(expanid, 16) || isAllZeros(expanid, 16)) {
    Serial.printf("\nInvalid EXPANID: %s\n", expanid);
    return false;
  } else {
    Serial.printf("\nValid EXPANID: %s\n", expanid);
  }

  return true;
}



// MAC ZigBee
void MAC_ZigBee(char *Mess) {
  char *p = strstr(Mess, "Mac Addr");
  int counter_response = 0;
  if (p != NULL) {
    while (p[counter_response] != '\0') {
      if (p[counter_response] == ':') {
        counter_response += 4;
        for (int i = 0; i < 16; i++) {
          MAC_Address[i] = p[counter_response++];
        }
        MAC_Address[16] = '\0';
        break;
      }
      counter_response++;
    }
  }
  if (MAC_Address[0] != '\0') {
    Serial.println("\nMAC address found ... ");
    if (strlen(MAC_Address) != 16) {
      Serial.println("\nMAC address found But not 16Digit ... ");
      getMAC = false;
      return;
    }
    for (size_t i = 0; i < 16; i++) {
      if (!isxdigit(MAC_Address[i])) {
        Serial.println("\nMAC address found But not Correct formela ... ");
        getMAC = false;
        return;
      }
    }
    Serial.print("\nMAC_ZigBee is : ");
    Serial.println(MAC_Address);
    Serial.println();

    ConvertStringMACtoCharr(MAC_Address);

    getMAC = true;
  } else {
    Serial.println("MAC address Not found ... ");
    getMAC = false;
    return;
  }
}


void ReschedualBelongDatabase(char *Index) {
  for (int i = 0; i < belongNodesNumber; i++) {
    if (Index[i] == 0) {
      continue;
    } else if (Index[i] == 1 && i < belongNodesNumber) {
      int y = i + 1;
      if (y == belongNodesNumber) {
        belongEndDevices[i].mac[0] = '\0';
        //devices[i].isNotBelongToThisCoo = false;
      }
      for (y; y < belongNodesNumber; y++) {
        strncpy(belongEndDevices[y - 1].mac, belongEndDevices[y].mac, 16);
        belongEndDevices[y].mac[0] = '\0';
      }
      //ThereisAnUpdate = true;
    }
  }
}

// bool parseMACsAndDeleting(const char *json) {
//   StaticJsonDocument<512> doc;  // حجم كافي للرسالة
//   char disconnectedIndex[belongNodesNumber] = { 0 };

//   DeserializationError err = deserializeJson(doc, json);
//   if (err) {
//     Serial.print("❌ JSON Parse failed: ");
//     Serial.println(err.c_str());
//     return false;
//   }

//   // نتاكد إن في array باسم "mac"
//   if (!doc.containsKey("mac")) {
//     Serial.println("❌ No mac key found in JSON");
//     return false;
//   }

//   JsonArray arr = doc["mac"].as<JsonArray>();

//   for (JsonVariant v : arr) {
//     const char *macStr = v.as<const char *>();
//     if (macStr == nullptr) continue;

//     //bool found = false;
//     for (int i = 0; i < belongNodesNumber; i++) {
//       if (strncmp(belongEndDevices[i].mac, macStr, 16) == 0) {
//         belongEndDevices[i].mac[0] == '\0';
//         disconnectedIndex[i] = 1;
//         ReschedualBelongDatabase(disconnectedIndex);
//         belongNodesNumber--;
//         break;
//       }
//     }
//   }

//   Serial.print("📊 Stored devices: ");
//   Serial.println(belongNodesNumber);
//   return true;
// }

bool parseMACsAndDeleting(const char *json) {
  StaticJsonDocument<512> doc;

  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.print("❌ JSON Parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc.containsKey("mac")) {
    Serial.println("❌ No mac key found in JSON");
    return false;
  }

  JsonArray arr = doc["mac"].as<JsonArray>();
  bool anyDeleted = false;

  for (JsonVariant v : arr) {
    const char *macStr = v.as<const char *>();
    if (macStr == nullptr) continue;

    // Clean MAC address
    char cleanMAC[17] = { 0 };
    int cleanIndex = 0;
    for (int i = 0; macStr[i] != '\0' && cleanIndex < 16; i++) {
      if (isxdigit(macStr[i])) {
        cleanMAC[cleanIndex++] = macStr[i];
      }
    }
    cleanMAC[16] = '\0';

    if (cleanIndex != 16) {
      Serial.print("⚠️ Invalid MAC format: ");
      Serial.println(macStr);
      continue;
    }

    // Find and remove the device
    for (int i = 0; i < belongNodesNumber; i++) {
      if (strncmp(belongEndDevices[i].mac, cleanMAC, 16) == 0) {
        Serial.print("🗑️ Removing device: ");
        Serial.println(cleanMAC);
        if (Flash_DeleteDeviceMAC(cleanMAC)) {
          belongEndDevices[i].mac[0] = '\0';
          belongEndDevices[i].added_to_EEPROM = false;
          anyDeleted = true;
          break;
        }
      }
    }
    // vTaskDelay(pdMS_TO_TICKS(5));
  }

  // If any devices were deleted, reorganize the array
  if (anyDeleted) {
    int writeIndex = 0;
    for (int readIndex = 0; readIndex < belongNodesNumber; readIndex++) {
      if (belongEndDevices[readIndex].mac[0] != '\0') {
        if (writeIndex != readIndex) {
          // Move this entry to the write position
          strcpy(belongEndDevices[writeIndex].mac, belongEndDevices[readIndex].mac);
          belongEndDevices[writeIndex].added_to_EEPROM = belongEndDevices[readIndex].added_to_EEPROM;
        }
        writeIndex++;
      }
      // vTaskDelay(pdMS_TO_TICKS(5));
    }
    belongNodesNumber = writeIndex;
  }

  Serial.print("📊 Stored devices after deletion: ");
  Serial.println(belongNodesNumber);

  // Debug: Print remaining devices
  for (int i = 0; i < belongNodesNumber; i++) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(belongEndDevices[i].mac);
    Serial.print(" (EEPROM: ");
    Serial.print(belongEndDevices[i].added_to_EEPROM ? "✅" : "❌");
    Serial.println(")");
    // vTaskDelay(pdMS_TO_TICKS(5));
  }
  return true;
}

bool parseMACs(const char *json) {
  StaticJsonDocument<512> doc;

  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.print("❌ JSON Parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc.containsKey("mac")) {
    Serial.println("❌ No mac key found in JSON");
    return false;
  }

  JsonArray arr = doc["mac"].as<JsonArray>();
  int addedCount = 0;

  for (JsonVariant v : arr) {
    const char *macStr = v.as<const char *>();
    if (macStr == nullptr) continue;

    // Clean MAC address
    char cleanMAC[17] = { 0 };
    int cleanIndex = 0;
    for (int i = 0; macStr[i] != '\0' && cleanIndex < 16; i++) {
      if (isxdigit(macStr[i])) {
        cleanMAC[cleanIndex++] = macStr[i];
      }
    }
    cleanMAC[16] = '\0';

    if (cleanIndex != 16) continue;

    // 🔴 CRITICAL: Check if already in belongEndDevices
    bool alreadyExists = false;
    for (int i = 0; i < belongNodesNumber; i++) {
      if (strcmp(belongEndDevices[i].mac, cleanMAC) == 0) {
        alreadyExists = true;
        Serial.print("⚠️ Device already exists, skipping: ");
        Serial.println(cleanMAC);
        break;
      }
      // vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (alreadyExists) continue;

    if (belongNodesNumber < MAX_DEVICES) {
      if (Flash_AddDeviceMAC(cleanMAC)) {
        // Add to RAM
        strncpy(belongEndDevices[belongNodesNumber].mac, cleanMAC, 16);
        belongEndDevices[belongNodesNumber].mac[16] = '\0';
        belongNodesNumber++;
        belongEndDevices[belongNodesNumber].added_to_EEPROM = true;
      }
    }
    // vTaskDelay(pdMS_TO_TICKS(5));
  }
  // vTaskDelay(pdMS_TO_TICKS(5));
  Serial.print("📊 Added ");
  Serial.print(addedCount);
  Serial.print(" new devices. Total: ");
  Serial.println(belongNodesNumber);
  return true;
}

bool parseCommand(char *mess) {

  if (deviceCount == 0) {
    Serial.println("There is no devices yet");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, mess);

  if (error) {
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
    return false;
  }

  const char *macJson = doc["mac"];
  if (!macJson) return false;

  char mac[17];
  strncpy(mac, macJson, sizeof(mac) - 1);
  mac[16] = '\0';

  for (uint8_t i = 0; i < deviceCount; i++) {

    if (strncmp(mac, devices[i].mac, 16) == 0) {

      Serial.print("\n✅ We found device index: ");
      Serial.println(i);

      // 🔎 Check payload BEFORE copying anything
      if (doc["payload"]) {
        const char *incomingPayload = doc["payload"].as<const char *>();

        if (strcmp(incomingPayload, "03") == 0 && devices[i].sleepAtr == false) {
          Serial.println("⛔ Payload 03 not allowed for this device (sleepAtr = false)");
          return false;  // ❌ Do NOT copy anything
        }
      }

      if (doc["userId"]) {
        strncpy(devices[i].userId, doc["userId"], sizeof(devices[i].userId) - 1);
        devices[i].userId[sizeof(devices[i].userId) - 1] = '\0';
      }

      if (doc["transactionId"]) {
        strncpy(devices[i].transactionId, doc["transactionId"], sizeof(devices[i].transactionId) - 1);
        devices[i].transactionId[sizeof(devices[i].transactionId) - 1] = '\0';
      }

      if (doc["propertyId"]) {
        strncpy(devices[i].propertyId, doc["propertyId"], sizeof(devices[i].propertyId) - 1);
        devices[i].propertyId[sizeof(devices[i].propertyId) - 1] = '\0';
      }

      if (doc["name"]) {
        strncpy(devices[i].name, doc["name"], sizeof(devices[i].name) - 1);
        devices[i].name[sizeof(devices[i].name) - 1] = '\0';
      }

      if (doc["payload"]) {
        snprintf(devices[i].payload,
                 sizeof(devices[i].payload),
                 "C,MAC:%s,%s\n",
                 devices[i].mac,
                 doc["payload"].as<const char *>());
      }

      // 🔎 طباعة كل القيم بعد النسخ
      Serial.println("------ Device Data ------");
      Serial.print("MAC: ");
      Serial.println(devices[i].mac);

      Serial.print("User ID: ");
      Serial.println(devices[i].userId);

      Serial.print("Transaction ID: ");
      Serial.println(devices[i].transactionId);

      Serial.print("Property ID: ");
      Serial.println(devices[i].propertyId);

      Serial.print("Name: ");
      Serial.println(devices[i].name);

      Serial.print("Payload to send: ");
      Serial.println(devices[i].payload);

      Serial.println("-------------------------");

      return true;
    }
    // vTaskDelay(pdMS_TO_TICKS(5));
  }

  Serial.println("❌ Device not found");
  return false;
}



void handleTopics() {
  //if(messageArrivedOnTopic == true){
  if (messageArriverOnMACCooTopic[0] != '\0' && messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes == false && commandTome == false) {
    Serial.print("\n\nHandling DataBaseBelong to nodes belong to our Coo with MAC: ");
    Serial.print(MAC_Address);
    Serial.print("\n\n");
    if (parseMACs(messageArriverOnMACCooTopic)) {
      if (belongNodesNumber > 0) {
        Serial.print("\n\nnodes belong to our Coo are: \n");
        for (int i = 0; i < belongNodesNumber; i++) {
          if (belongEndDevices[i].mac[0] != '\0') {
            Serial.print(i);
            Serial.print(", ");
            Serial.println(belongEndDevices[i].mac);
          }
        }
        Serial.println();
      }
    }
    messageArriverOnMACCooTopic[0] = '\0';
    messageArrivedOnBleongsNodes = false;
  } else if (messageArriverOnMACCooTopic[0] != '\0' && !messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes && commandTome == false) {
    Serial.print("\n\nHandling deleting a Node from belongDatabase to nodes belong to our Coo with MAC: ");
    Serial.print(MAC_Address);
    Serial.print("\n\n");
    if (parseMACsAndDeleting(messageArriverOnMACCooTopic)) {
      if (belongNodesNumber > 0) {
        Serial.print("\n\nnodes belong to our Coo are: \n");
        for (int i = 0; i < belongNodesNumber; i++) {
          if (belongEndDevices[i].mac[0] != '\0') {
            Serial.print(i);
            Serial.print(", ");
            Serial.println(belongEndDevices[i].mac);
          }
        }
        Serial.println();
      }
    }
    messageArriverOnMACCooTopic[0] = '\0';
    messageArrivedOnDeletingOnBleongsNodes = false;
  } else if (messageArriverOnMACCooTopic[0] != '\0' && !messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes == false && commandTome == true) {
    Serial.print("\n\nHandleing the command topic.\n");
    if (parseCommand(messageArriverOnMACCooTopic)) {
      messageArriverOnMACCooTopic[0] = '\0';
    } else {
      Serial.print("\n\Message format is not correct:\n");
      messageArriverOnMACCooTopic[0] = '\0';
    }
    commandTome = false;
  } else {
    messageArriverOnMACCooTopic[0] = '\0';
  }
}


// Send Command
void CommToZigBee(char *Mess, bool checkCooNet, int timing) {
  response[0] = '\0';
  counter_response = 0;
  bool messageRecieve = false;
  TickType_t lastTimeforResponse = xTaskGetTickCount();
  if (strncmp(Mess, "ATRB", 4) == 0) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
          response[counter_response] = '\0';
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Serial.println();
        Serial.println();
      } else continue;
    }
  } else if (strncmp(Mess, "ATIF", 4) == 0) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
          response[counter_response] = '\0';
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;

        if (checkCooNet) {
          checkCooNetworkSetting = checkCooNetwork(response);
        } else {
          MAC_ZigBee(response);
        }

        Serial.println();
        break;
      } else {
        //Serial.print("OK not found on command to Zigbee ATIF");
        continue;
      }
    }
  } else {
    lastTimeforResponse = xTaskGetTickCount();
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    // TickType_t lastTimeforResponse = xTaskGetTickCount();
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
          response[counter_response] = '\0';
        }
        //Serial.write(c);  // للطباعة على الشاشة بدون تشويش
        if (c == '\n') {
          if (strncmp(Mess, "+++", 3) == 0) {
            if (strncmp(response, "Error", 5) == 0 || strncmp(response, "Enter AT Mode", 13) == 0) {
              //Serial.print(response);
              messageRecieve = true;
              hardwareZigBee = true;
              Serial.println();
              break;
            } else continue;
          } else if (strncmp(Mess, "ATTM", 4) == 0) {
            if (strstr(response, "OK") != NULL) {
              messageRecieve = true;
              Serial.println();
              break;
            } else continue;
          } else if (strncmp(Mess, "ATDA", 4) == 0) {
            if (strstr(response, "OK") != NULL) {
              messageRecieve = true;
              Serial.println();
              break;
            } else continue;
          } else if (strncmp(Mess, "ATPA", 4) == 0) {
            if (strstr(response, "OK") != NULL) {
              messageRecieve = true;
              Serial.println();
              break;
            } else continue;
          } else if (strncmp(Mess, "ATDT", 4) == 0) {
            if (strstr(response, "OK") != NULL) {
              messageRecieve = true;
              Serial.println();
              break;
            } else continue;
          }
        }
      }

      if (response[0] == '\0' && (xTaskGetTickCount() - lastTimeforResponse >= 1000)) {
        Serial.println("There is no hardware please check .... \n");
        hardwareZigBee = false;
        ZigBee_init = false;
        return;
        // goto RepateSending;
      }
    }
  }
}

// init ZigBee
void initZigBee() {
  /*strncpy(devices[0].mac, "00158d00099ab369", 16);
  devices[0].TimerNode = millis();
  strncpy(devices[1].mac, "00158d00099ab555", 16);
  devices[1].TimerNode = millis();
  deviceCount = 2;
  Serial.print("ZigBee_init ");
  Serial.println("true... ");
  ZigBee_init = true;*/
  while (!hardwareZigBee) {
    CommToZigBee("+++\r\n", false, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  while (getMAC == false) {
    CommToZigBee("ATIF\r\n", false, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  CommToZigBee("ATIF\r\n", true, 0);

  if (checkCooNetworkSetting) {

  } else {
    CommToZigBee("ATPA1\r\n", false, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    CommToZigBee("ATRB\r\n", false, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    while (1) {
      CommToZigBee("ATIF\r\n", true, 0);
      vTaskDelay(pdMS_TO_TICKS(1000));

      if (checkCooNetworkSetting) {
        break;
      } else {
        CommToZigBee("ATRB\r\n", false, 0);
        vTaskDelay(pdMS_TO_TICKS(10000));
      }
    }
  }

  Serial.println();
  CommToZigBee("ATDT\r\n", false, 0);
  Serial.print("ZigBee_init ");
  Serial.println("true... ");
  ZigBee_init = true;
  response[0] = '\0';
  counter_response = 0;
}


void send_saved_messages_when_net_is_on() {
  if (MQTT_FlageOK && DHCP_FlageOK) {
    for (int i = 0; i < MAX_DEVICES + 20; i++) {
      if (messages_saved[i].message_saved_duto_internet_off[0] != '\0') {
        if (strstr(messages_saved[i].message_saved_duto_internet_off, "smartlock") != NULL) {
          publishDevices("smartlock", messages_saved[i].message_saved_duto_internet_off, sizeof(messages_saved[i].message_saved_duto_internet_off));
          messages_saved[i].message_saved_duto_internet_off[0] = '\0';
        } else {
        }
      }
    }
  }
}

StaticJsonDocument<512> publish;
void publish_on_topics(char *mac, uint8_t indexOfNode, bool disconnected, bool openDoorFailure, bool passwordnotset, bool batteryCheckFailed, bool batteryCheck, bool openDoorSuccess, bool passwordset) {
  publish.clear();
  // Create a character array to hold the serialized JSON string
  char jsonBuffer[512];
  if (strncmp(devices[indexOfNode].device_type, types.smartlocks, sizeof(types.smartlocks)) == 0) {
    publish["device"] = "smartlock";
  } else {
  }
  if (MQTT_FlageOK && DHCP_FlageOK) {
    if (disconnected) {
      publish["level"] = eventDevice.failure;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "This node is disconnected from a network";
    } else if (openDoorFailure) {
      publish["level"] = eventDevice.failure;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "door didn't open successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (batteryCheckFailed) {
      publish["level"] = eventDevice.failure;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "Battery didn't checked successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (batteryCheck) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "Battery read successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
      publish["payload"] = devices[indexOfNode].batterlevel;
    } else if (openDoorSuccess) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "door unlocked successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (passwordset) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "password set successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    }
    // Serialize the JSON document into the buffer
    size_t len = serializeJson(publish, jsonBuffer);
    Serial.write(jsonBuffer, len);
    Serial.println();
    if (len == 0) return;
    if (strncmp(devices[indexOfNode].device_type, types.smartlocks, sizeof(types.smartlocks)) == 0) {
      publishDevices("smartlock", jsonBuffer, len);
    }
  } else if (!MQTT_FlageOK || !DHCP_FlageOK) {
    if (disconnected) {
      publish["level"] = eventDevice.failure;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "This node is disconnected from a network";
    } else if (openDoorFailure) {
      publish["level"] = eventDevice.failure;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "door didn't open successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (batteryCheckFailed) {
      publish["level"] = eventDevice.failure;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "Battery didn't checked successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (batteryCheck) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "Battery read successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
      publish["payload"] = devices[indexOfNode].batterlevel;
    } else if (openDoorSuccess) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "door unlocked successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    } else if (passwordset) {
      publish["level"] = eventDevice.success;
      publish["transactionId"] = devices[indexOfNode].transactionId;
      publish["mac"] = devices[indexOfNode].mac;
      publish["message"] = "password set successfully";
      publish["userId"] = devices[indexOfNode].userId;
      publish["propertyId"] = devices[indexOfNode].propertyId;
      publish["name"] = devices[indexOfNode].name;
    }
    // Serialize the JSON document into the buffer
    size_t len = serializeJson(publish, jsonBuffer);
    Serial.write(jsonBuffer, len);
    Serial.println();
    for (int i = 0; i < MAX_DEVICES + 20; i++) {
      if (messages_saved[i].message_saved_duto_internet_off[0] == '\0') {
        strncpy(messages_saved[i].message_saved_duto_internet_off, jsonBuffer, len);
        break;
      }
    }
  }
}

void ReschedualNotBelongDatabase(char *Index) {
  for (int i = 0; i < notbelongNodesNumber; i++) {
    if (Index[i] == 0) {
      continue;
    } else if (Index[i] == 1 && i < notbelongNodesNumber) {
      int y = i + 1;
      if (y == notbelongNodesNumber) {
        notbelongEndDevices[i].mac[0] = '\0';
        notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong = false;
        //devices[i].isNotBelongToThisCoo = false;
      }
      for (y; y < notbelongNodesNumber; y++) {
        strncpy(notbelongEndDevices[y - 1].mac, notbelongEndDevices[y].mac, 16);
        notbelongEndDevices[y].mac[0] = '\0';
        notbelongEndDevices[y - 1].sendTheresponseToNodeThatNotbelong = notbelongEndDevices[y].sendTheresponseToNodeThatNotbelong;
        notbelongEndDevices[y].sendTheresponseToNodeThatNotbelong = false;
      }
      //ThereisAnUpdate = true;
    }
  }
}

void HandlEveryIncoming() {
  char responseBuffer[1024];
  responseBuffer[0] = '\0';
  //buffer[0] = '\0';


  while (Serial2.available()) {
    char c = Serial2.read();
    int len = strlen(buffer);
    if (len < sizeof(buffer) - 1) {
      buffer[len] = c;
      buffer[len + 1] = '\0';
    }
  }

  char *newlinePtr;
  while ((newlinePtr = strchr(buffer, '\n')) != NULL) {
    int lineLength = newlinePtr - buffer;

    char line[128];
    line[0] = '\0';
    strncpy(line, buffer, lineLength);
    line[lineLength] = '\0';
    Serial.println(line);


    // Remove processed line from buffer
    memmove(buffer, newlinePtr + 1, strlen(newlinePtr + 1) + 1);

    // Remove trailing '\r' or space
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ')) {
      line[--len] = '\0';
    }

    // === Handle Format 1: OP:N, MA:<MAC>, SL:1 ===
    if (strncmp(line, "N,MAC:", 6) == 0) {
      // Serial.println(line);
      char device_type[2] = { 0 };
      char mac[17];
      bool findSleep = false;
      strncpy(mac, line + 6, 16);
      mac[16] = '\0';

      char *slPtr = strstr(line, ",SL:");
      if (slPtr != NULL) {
        char sleepByte = slPtr[4];  // الرقم بعد SL:
        device_type[0] = slPtr[5];  // الحرف بعده

        if (sleepByte == '1') {
          Serial.println("\ndevice in mode sleep");
          findSleep = true;
        } else {
          findSleep = false;
        }
      }

      bool validMAC = true;
      for (int i = 0; i < 16; i++) {
        if (!isxdigit(mac[i])) {
          validMAC = false;
          break;
        }
      }

      if (!validMAC) {
        Serial.print("Invalid MAC received: ");
        Serial.println(mac);
        continue;
      }

      bool found = false;
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0 && deviceCount > 0) {
          strncpy(devices[i].message, line, sizeof(devices[i].message) - 1);
          devices[i].message[sizeof(devices[i].message) - 1] = '\0';
          if (device_type[0] == 'D') {
            Serial.println("\ndevice is smart lock");
            strcpy(devices[i].device_type, types.smartlocks);
          }
          strcat(responseBuffer, "N,MAC:");
          strcat(responseBuffer, devices[i].mac);
          strcat(responseBuffer, ",00\n");
          devices[i].TimerNode = xTaskGetTickCount();
          devices[i].sleepAtr = findSleep;
          Serial.print(responseBuffer);
          Serial2.print(responseBuffer);
          responseBuffer[0] = '\0';
          if (devices[i].payload[0] != '\0' && devices[i].sendMAcEndDeviceToBroker == true) {
            responseBuffer[0] = '\0';
            Serial.print("\n\nThere is payload message for device: ");
            Serial.print(i);
            Serial.print(" ,MAC: ");
            Serial.print(devices[i].mac);
            Serial.print(" ,Status is: Failure");
            if (strstr(devices[i].payload, ",01")) {
              publish_on_topics(devices[i].mac, i, false, true, false, false, false, false, false);
            } else if (strstr(devices[i].payload, ",02")) {
              publish_on_topics(devices[i].mac, i, false, false, true, false, false, false, false);
            } else if (strstr(devices[i].payload, ",03")) {
              publish_on_topics(devices[i].mac, i, false, false, false, true, false, false, false);
            }

            devices[i].payload[0] = '\0';
            devices[i].transactionId[0] = '\0';
            devices[i].userId[0] = '\0';
            devices[i].propertyId[0] = '\0';
            devices[i].name[0] = '\0';
          }
          found = true;
          break;
        }
      }

      if (!found && deviceCount < MAX_DEVICES && belongNodesNumber > 0) {
        bool foundInOurBelongNodes = false;
        for (int i = 0; i < belongNodesNumber; i++) {
          if (belongEndDevices[i].mac[0] != '\0') {
            if (strncmp(belongEndDevices[i].mac, mac, 16) == 0) {
              foundInOurBelongNodes = true;
              break;
            }
          }
        }

        if (foundInOurBelongNodes) {

          strncpy(devices[deviceCount].mac, mac, 16);
          devices[deviceCount].mac[16] = '\0';
          devices[deviceCount].TimerNode = xTaskGetTickCount();
          strncpy(devices[deviceCount].message, line, sizeof(devices[deviceCount].message) - 1);
          devices[deviceCount].message[sizeof(devices[deviceCount].message) - 1] = '\0';

          if (device_type[0] == 'D') {
            Serial.println("\ndevice is smart lock");
            strcpy(devices[deviceCount].device_type, types.smartlocks);
          }
          devices[deviceCount].TimerNode = xTaskGetTickCount();
          devices[deviceCount].sleepAtr = findSleep;

          strcat(responseBuffer, "N,MAC:");
          strcat(responseBuffer, devices[deviceCount].mac);
          strcat(responseBuffer, ",00\n");
          deviceCount = deviceCount + 1;
        } else {
          Serial.print("\n\nWait you Node To send you to the Broker,\nand resend the response to what MAC Coo you belonge to...\n\n");
        }
      }
    } else if (strncmp(line, "S,MAC:", 6) == 0) {
      //Serial.println(line);
      char mac[17];
      strncpy(mac, line + 6, 16);
      mac[16] = '\0';

      bool validMAC = true;
      for (int i = 0; i < 16; i++) {
        if (!isxdigit(mac[i])) {
          validMAC = false;
          break;
        }
      }

      if (!validMAC) {
        Serial.print("Invalid MAC received: ");
        Serial.println(mac);
        continue;
      }
      //Serial.println(mac);
      for (int z = 0; z < notbelongNodesNumber; z++) {
        if (strncmp(notbelongEndDevices[z].mac, mac, 16) == 0) {
          Serial.print("\n\nyou are now not belong to me\n\n");
          notbelongEndDevices[z].sendTheresponseToNodeThatNotbelong = true;
          break;
        }
      }
      bool findinDevices = false;
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0) {
          findinDevices = true;
          Serial.print("Add new Time for device: ");
          Serial.print(i);
          Serial.print(" ,MAC: ");
          Serial.print(devices[i].mac);
          Serial.print(" ");
          devices[i].TimerNode = xTaskGetTickCount();
          Serial.print(" ");
          Serial.println(devices[i].TimerNode);
          if (devices[i].payload[0] != '\0' && devices[i].sendMAcEndDeviceToBroker == true && devices[i].sleepAtr == true) {
            Serial.print("There is payload message for device: ");
            Serial.print(i);
            Serial.print(" ,MAC: ");
            Serial.print(devices[i].mac);
            Serial.println();
            Serial.print("Message is: ");
            Serial.print(devices[i].payload);
            Serial2.print(devices[i].payload);

            if (strstr(devices[i].payload, ",01") != NULL) {
              publish_on_topics(devices[i].mac, i, false, false, false, false, false, true, false);
              devices[i].payload[0] = '\0';
              devices[i].transactionId[0] = '\0';
              devices[i].userId[0] = '\0';
              devices[i].propertyId[0] = '\0';
              devices[i].name[0] = '\0';
            } else if (strstr(devices[i].payload, ",02") != NULL) {
              publish_on_topics(devices[i].mac, i, false, false, false, false, false, false, true);
              devices[i].payload[0] = '\0';
              devices[i].transactionId[0] = '\0';
              devices[i].userId[0] = '\0';
              devices[i].propertyId[0] = '\0';
              devices[i].name[0] = '\0';
            }
          }
          break;
        }
      }
      if (!findinDevices) {
        Serial.print("\nThere is a node working see if it is in the belong data base\n");
        bool foundInOurBelongNodes = false;
        for (int i = 0; i < belongNodesNumber; i++) {
          if (belongEndDevices[i].mac[0] != '\0') {
            if (strncmp(belongEndDevices[i].mac, mac, 16) == 0) {
              foundInOurBelongNodes = true;
              break;
            }
          }
        }
        if (foundInOurBelongNodes) {
          strcat(responseBuffer, "R,MAC:");
          strcat(responseBuffer, mac);
          strcat(responseBuffer, ",FF\n");
          // deviceCount = deviceCount + 1;
        }
      }
    } else if (strncmp(line, "A,MAC:", 6) == 0) {
      //Serial.println(line);
      char mac[17];
      strncpy(mac, line + 6, 16);
      mac[16] = '\0';

      bool validMAC = true;
      for (int i = 0; i < 16; i++) {
        if (!isxdigit(mac[i])) {
          validMAC = false;
          break;
        }
      }

      if (!validMAC) {
        Serial.print("Invalid MAC received: ");
        Serial.println(mac);
        continue;
      }
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0 && devices[i].sleepAtr == false && devices[i].waitingReceiveOKfromNodewithoutSleep == true) {
          Serial.print("Recieved OK form device: ");
          Serial.print(i);
          Serial.print(" ,MAC: ");
          Serial.println(devices[i].mac);

          if (strstr(devices[i].payload, ",01") != NULL) {
            publish_on_topics(devices[i].mac, i, false, false, false, false, false, true, false);
          } else if (strstr(devices[i].payload, ",02") != NULL) {
            publish_on_topics(devices[i].mac, i, false, false, false, false, false, false, true);
          }

          devices[i].payload[0] = '\0';
          devices[i].transactionId[0] = '\0';
          devices[i].userId[0] = '\0';
          devices[i].propertyId[0] = '\0';
          devices[i].name[0] = '\0';
          devices[i].waitingReceiveOKfromNodewithoutSleep = false;
        }
      }
    } else if (strncmp(line, "B,MAC:", 6) == 0) {
      char mac[17];
      strncpy(mac, line + 6, 16);
      mac[16] = '\0';

      // التحقق من صحة MAC
      bool validMAC = true;
      for (int i = 0; i < 16; i++) {
        if (!isxdigit(mac[i])) {
          validMAC = false;
          break;
        }
      }
      if (!validMAC) {
        Serial.print("Invalid MAC received: ");
        Serial.println(mac);
        continue;
      }

      // البحث عن الجهاز الصحيح
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0 && devices[i].sleepAtr == true) {
          Serial.print("Received Battery State from device: ");
          Serial.print(i);
          Serial.print(" ,MAC: ");
          Serial.println(devices[i].mac);

          // -------- استخراج مستوى البطارية بعد آخر فاصلة --------
          char *lastComma = strrchr(line, ',');  // العثور على آخر فاصلة
          if (lastComma != NULL) {
            lastComma++;  // تجاوز الفاصلة

            // إزالة أي مسافات أو \r أو \n من البداية
            while (*lastComma == ' ' || *lastComma == '\r' || *lastComma == '\n') lastComma++;

            // نسخ البطارية مع نهاية السلسلة \0
            strncpy(devices[i].batterlevel, lastComma, sizeof(devices[i].batterlevel) - 1);
            devices[i].batterlevel[sizeof(devices[i].batterlevel) - 1] = '\0';

            // إزالة أي \r أو \n في النهاية
            int len = strlen(devices[i].batterlevel);
            while (len > 0 && (devices[i].batterlevel[len - 1] == '\r' || devices[i].batterlevel[len - 1] == '\n' || devices[i].batterlevel[len - 1] == ' ')) {
              devices[i].batterlevel[--len] = '\0';
            }
          }

          // نشر البيانات على المواضيع
          publish_on_topics(devices[i].mac, i, false, false, false, false, true, false, false);

          // إعادة تعيين الحقول
          devices[i].payload[0] = '\0';
          devices[i].transactionId[0] = '\0';
          devices[i].userId[0] = '\0';
          devices[i].propertyId[0] = '\0';
          devices[i].name[0] = '\0';
          // لا تعيد تعيين batterlevel لأنها الآن تحتوي على القيمة الصحيحة
          devices[i].sleepAtr = true;
        }
      }
    } else {
      buffer[0] = '\0';
    }
  }

  //Sending Payloads to devices without Sleep Mode
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].payload[0] != '\0' && devices[i].sendMAcEndDeviceToBroker == true && devices[i].sleepAtr == false && devices[i].waitingReceiveOKfromNodewithoutSleep == false) {

      Serial.print("\nThere is payload message for this device without sleep mode: ");
      Serial.print(i);
      Serial.print(" ,MAC: ");
      Serial.print(devices[i].mac);
      Serial.print("\nMessage is: ");
      Serial.print(devices[i].payload);
      delay(50);
      Serial2.print(devices[i].payload);
      devices[i].waitingReceiveOKfromNodewithoutSleep = true;
      devices[i].sleepAtr = false;
    }
  }



  if (strlen(responseBuffer) > 0) {
    Serial.print(responseBuffer);
    Serial2.print(responseBuffer);
  }

  // Print device list every 10s
  if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(10000)) {

    Serial.println("\n\n=== Devices ===");
    if (deviceCount == 0) {
      Serial.println("There is no devices\n");
    } else {
      Serial.println();
      for (int i = 0; i < deviceCount; i++) {
        Serial.print("Device ");
        Serial.print(i);
        Serial.print(" [");
        Serial.print(devices[i].mac);
        Serial.print("], ");
        Serial.print("Sleep:[");
        Serial.print(devices[i].sleepAtr);
        Serial.println("].");
      }
      Serial.println();
    }
    lastReceiveTime = xTaskGetTickCount();
  }
}


void ReschedualDevices(char *Index) {
  for (int i = 0; i < deviceCount; i++) {
    if (Index[i] == 0) {
      continue;
    } else if (Index[i] == 1 && i < deviceCount) {
      int y = i + 1;
      if (y == deviceCount) {
        devices[i].mac[0] = '\0';
        devices[i].message[0] = '\0';
        devices[i].payload[0] = '\0';
        devices[i].transactionId[0] = '\0';
        devices[i].userId[0] = '\0';
        devices[i].propertyId[0] = '\0';
        devices[i].name[0] = '\0';
        devices[i].TimerNode = 0;
        devices[i].sendMAcEndDeviceToBroker = false;
        devices[i].sleepAtr = false;
        devices[i].device_type[0] = '\0';
        devices[i].waitingReceiveOKfromNodewithoutSleep = false;
        //devices[i].isNotBelongToThisCoo = false;
        return;
      }
      for (y; y < deviceCount; y++) {
        strncpy(devices[y - 1].mac, devices[y].mac, 16);
        devices[y].mac[0] = '\0';
        strncpy(devices[y - 1].message, devices[y].message, sizeof(devices[y].message) - 1);
        devices[y].message[0] = '\0';
        strncpy(devices[y - 1].payload, devices[y].payload, sizeof(devices[y].payload) - 1);
        devices[y].payload[0] = '\0';
        devices[y - 1].TimerNode = devices[y].TimerNode;
        devices[y].TimerNode = 0;
        devices[y - 1].sendMAcEndDeviceToBroker = devices[y].sendMAcEndDeviceToBroker;
        devices[y].sendMAcEndDeviceToBroker = false;
        strncpy(devices[y - 1].transactionId, devices[y].transactionId, sizeof(devices[y].transactionId) - 1);
        devices[y].transactionId[0] = '\0';
        strncpy(devices[y - 1].userId, devices[y].userId, sizeof(devices[y].userId) - 1);
        devices[y].userId[0] = '\0';
        strncpy(devices[y - 1].propertyId, devices[y].propertyId, sizeof(devices[y].propertyId) - 1);
        devices[y].propertyId[0] = '\0';
        strncpy(devices[y - 1].name, devices[y].name, sizeof(devices[y].name) - 1);
        devices[y].name[0] = '\0';
        devices[y - 1].sleepAtr = devices[y].sleepAtr;
        devices[y].sleepAtr = false;
        strncpy(devices[y - 1].device_type, devices[y].device_type, sizeof(devices[y].device_type) - 1);
        devices[y].device_type[0] = '\0';
        devices[y - 1].waitingReceiveOKfromNodewithoutSleep = devices[y].waitingReceiveOKfromNodewithoutSleep;
        devices[y].waitingReceiveOKfromNodewithoutSleep = false;
      }
    }
  }
}


void addNodeToNotbelongeDatabase(char *mac) {
  bool found = false;
  for (int i = 0; i < notbelongNodesNumber; i++) {
    if (strncmp(notbelongEndDevices[i].mac, mac, 16) == 0) {
      found = true;
      break;
    }
  }
  if (found) {
    return;
  } else {
    strncpy(notbelongEndDevices[notbelongNodesNumber].mac, mac, 16);
    notbelongNodesNumber++;
    Serial.print("\n\nadded To our database for not belonge\n\n");
  }
  for (int i = 0; i < notbelongNodesNumber; i++) {
    Serial.print("\n\nNot Belong data: ");
    Serial.print(notbelongNodesNumber);
    Serial.print("\n\n");
    Serial.print(i);
    Serial.print(", ");
    Serial.println(notbelongEndDevices[i].mac);
  }
  Serial.print("\n\n");
}

void TimersForNodes() {

  // if (notbelongNodesNumber > 0) {
  //   char disconnectedIndex[notbelongNodesNumber] = { 0 };
  //   for (int i = 0; i < notbelongNodesNumber; i++) {
  //     if (notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong == true) {
  //       Serial.print("\n\nWe must send it to broker now\n\n");
  //       notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong = false;
  //       notbelongEndDevices[i].mac[0] = '\0';
  //       disconnectedIndex[i] = 1;
  //       ReschedualNotBelongDatabase(disconnectedIndex);
  //       notbelongNodesNumber--;
  //       break;
  //     }
  //   }
  // }
  // if (belongNodesNumber > 0) {
  //   char disconnectedIndex[deviceCount] = { 0 };
  //   for (int i = 0; i < deviceCount; i++) {
  //     bool found = false;
  //     for (int z = 0; z < belongNodesNumber; z++) {
  //       if (devices[i].mac[0] != '\0') {
  //         if (strncmp(belongEndDevices[z].mac, devices[i].mac, 16) == 0) {
  //           found = true;
  //           break;
  //         }
  //       }
  //     }
  //     if (!found) {
  //       Serial.print("\nWe finde a node is now not belonging to our COO with its MAC: ");
  //       Serial.print("Device ");
  //       Serial.print(i);
  //       Serial.print(" [");
  //       Serial.print(devices[i].mac);
  //       Serial.print("]  ");

  //       if (devices[i].payload[0] != '\0') {
  //         //commandReplyFailure(devices[i].mac, i);
  //       }

  //       addNodeToNotbelongeDatabase(devices[i].mac);
  //       devices[i].mac[0] = '\0';
  //       devices[i].message[0] = '\0';
  //       devices[i].payload[0] = '\0';
  //       devices[i].transactionId[0] = '\0';
  //       devices[i].userId[0] = '\0';
  //       devices[i].propertyId[0] = '\0';
  //       devices[i].name[0] = '\0';
  //       devices[i].TimerNode = 0;
  //       devices[i].sendMAcEndDeviceToBroker = false;
  //       devices[i].sleepAtr = false;
  //       devices[i].device_type[0] = '\0';
  //       devices[i].waitingReceiveOKfromNodewithoutSleep = false;

  //       disconnectedIndex[i] = 1;
  //       ReschedualDevices(disconnectedIndex);
  //       deviceCount = deviceCount - 1;
  //       break;
  //     }
  //   }
  // }
  if (deviceCount != 0) {
    char disconnectedIndex[deviceCount] = { 0 };
    //bool Reschedual = false;
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].mac[0] != '\0') {
        if ((xTaskGetTickCount() - devices[i].TimerNode) >= pdMS_TO_TICKS(timeout)) {
          //Serial.println( xTaskGetTickCount() - devices[i].TimerNode);
          Serial.print("Device ");
          Serial.print(i);
          Serial.print(" [");
          Serial.print(devices[i].mac);
          Serial.print("]  ");
          Serial.println("Disconnected");

          publish_on_topics(devices[i].mac, i, true, false, false, false, false, false, false);

          if (strstr(devices[i].payload, ",01")) {
            publish_on_topics(devices[i].mac, i, false, true, false, false, false, false, false);
          } else if (strstr(devices[i].payload, ",02")) {
            publish_on_topics(devices[i].mac, i, false, false, true, false, false, false, false);
          } else if (strstr(devices[i].payload, ",03")) {
            publish_on_topics(devices[i].mac, i, false, false, false, true, false, false, false);
          }

          devices[i].mac[0] = '\0';
          devices[i].message[0] = '\0';
          devices[i].payload[0] = '\0';
          devices[i].transactionId[0] = '\0';
          devices[i].userId[0] = '\0';
          devices[i].propertyId[0] = '\0';
          devices[i].name[0] = '\0';
          devices[i].TimerNode = 0;
          devices[i].sendMAcEndDeviceToBroker = false;
          devices[i].sleepAtr = false;
          devices[i].device_type[0] = '\0';
          devices[i].waitingReceiveOKfromNodewithoutSleep = false;

          disconnectedIndex[i] = 1;
          ReschedualDevices(disconnectedIndex);
          deviceCount = deviceCount - 1;
          break;
        }
      }
    }
  }
}

void sendDataBASE_AfterConnectionIsDone() {
  StaticJsonDocument<256> doc;
  // Create a character array to hold the serialized JSON string
  char jsonBufferUpdate[512];

  // نعمل مصفوفة تحت اسم "database"
  JsonArray mac = doc.createNestedArray("mac");

  if (deviceCount == 0) {
    //doc["mac"] = mac.add("");//NULL;
  } else {
    for (int i = 0; i < deviceCount; i++) {
      // كل mac نضيفه كـ string جديد في المصفوفة
      mac.add(devices[i].mac);

      // نحدد إنو تم الإرسال
      devices[i].sendMAcEndDeviceToBroker = true;
    }
  }

  // نرسل JSON
  serializeJson(doc, jsonBufferUpdate);
  PublishDataBase(jsonBufferUpdate);

  sendDatabase = true;
}

void SendNewMacEndDevice() {
  StaticJsonDocument<256> doc;
  if (deviceCount > 0) {
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].sendMAcEndDeviceToBroker == false) {

        char number[3];
        itoa(i, number, 10);
        doc["device"] = number;
        doc["mac"] = devices[i].mac;
        doc["device_type"] = devices[i].device_type;
        doc["sleep"] = devices[i].sleepAtr;

        // Create a character array to hold the serialized JSON string
        char jsonBuffer[256];

        // Serialize the JSON document into the buffer
        size_t len = serializeJson(doc, jsonBuffer);
        Serial.print("\nPublish the MAC to brocker on a newNode topic.\n");
        Serial.write(jsonBuffer, len);
        Serial.println();
        if (len == 0) return;
        if (MQTT_FlageOK) {
          PublishOnNewNodeAdded(jsonBuffer, len);
          devices[i].sendMAcEndDeviceToBroker = true;
        }
      }
    }
  }
}

extern bool SendLogMACCooToBroker;
//bool weWereHavingBelongDataOnce = false;
#define LED_PIN PB0

void indicators() {

  if (ZigBee_init == true && !MQTT_FlageOK && !DHCP_FlageOK) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(PB1, LOW);
    vTaskDelay(250);
  } else if (ZigBee_init == true && !MQTT_FlageOK && DHCP_FlageOK) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(PB1, HIGH);
    vTaskDelay(250);
  } else if (ZigBee_init == true && MQTT_FlageOK && DHCP_FlageOK && belongNodesNumber > 0) {  // && MQTT_FlageOK && DHCP_FlageOK){
    //Serial.println(belongNodesNumber);
    //weWereHavingBelongDataOnce = true;
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(250);
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(250);
  } else if (ZigBee_init == true && MQTT_FlageOK && DHCP_FlageOK && belongNodesNumber == 0) {  // && weWereHavingBelongDataOnce){// && MQTT_FlageOK && DHCP_FlageOK){
    //Serial.println(belongNodesNumber);
    //SendLogMACCooToBroker = true;
    /*if(weWereHavingBelongDataOnce){
      SendLogMACCooToBroker = false;
      weWereHavingBelongDataOnce = false;
    }*/
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(250);
  } else if (!ZigBee_init && !MQTT_FlageOK && !DHCP_FlageOK) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(PB1, LOW);
    vTaskDelay(250);
  }
}


void make_string_of_belong_nodes_and_send_them() {
  // Create JSON document with all MACs
  StaticJsonDocument<512> doc;

  if (belongNodesNumber == 0) {
    PublishMACCooRoBroker(NULL, 0);
  } else {
    // Create array under "mac" key
    JsonArray macArray = doc.createNestedArray("mac");

    // Add all MACs from belongEndDevices
    for (int i = 0; i < belongNodesNumber; i++) {
      if (belongEndDevices[i].mac[0] != '\0') {
        macArray.add(belongEndDevices[i].mac);
      }
    }

    // Serialize the ENTIRE document (not just the array)
    char jsonBuffer[512];
    size_t len = serializeJson(doc, jsonBuffer);  // ← This serializes {"mac":[...]}

    Serial.print("📤 Publishing: ");
    Serial.println(jsonBuffer);

    if (len > 0) {
      PublishMACCooRoBroker(jsonBuffer, len);
    }
  }
}

void load_belong_devices_from_EEPROM() {
  Serial.println("\n========== LOADING FLASH DEVICES ==========");

  char macs[MAX_DEVICES][17];

  // This will load from flash AND update cachedCounter
  uint8_t flashCount = Flash_LoadAllMACs(macs);
  // Flash_LoadAllMACs();
  // uint8_t flashCount = 0;
  if (flashCount == 0) {
    Serial.println("📭 No devices in Flash to load");
    belongNodesNumber = 0;
    return;
  }

  Serial.print("📀 Found ");
  Serial.print(flashCount);
  Serial.println(" devices in Flash");

  // Clear existing belongEndDevices
  memset(belongEndDevices, 0, sizeof(belongEndDevices));
  belongNodesNumber = 0;

  // Load MACs from flash into belongEndDevices
  for (uint8_t i = 0; i < flashCount; i++) {
    if (macs[i][0] == '\0') continue;

    // Add to belongEndDevices
    strncpy(belongEndDevices[belongNodesNumber].mac, macs[i], 16);
    belongEndDevices[belongNodesNumber].mac[16] = '\0';
    belongEndDevices[belongNodesNumber].added_to_EEPROM = true;

    Serial.print("  ✅ Loaded [");
    Serial.print(belongNodesNumber);
    Serial.print("]: ");

    // Format MAC with colons for display
    for (int j = 0; j < 16; j++) {
      Serial.print(belongEndDevices[belongNodesNumber].mac[j]);
      if (j % 2 == 1 && j < 15) {
        Serial.print(":");
      }
    }
    Serial.println();

    belongNodesNumber++;
  }

  Serial.print("\n📊 Successfully loaded ");
  Serial.print(belongNodesNumber);
  Serial.print("/");
  Serial.print(flashCount);
  Serial.println(" devices into belongEndDevices");

  // Debug: Show what's in cache
  Serial.println("\n📋 Current cache contents:");
  for (int i = 0; i < flashCount; i++) {
    Serial.print("  Cache[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.println(macs[i]);
  }

  Serial.println("============================================\n");
}