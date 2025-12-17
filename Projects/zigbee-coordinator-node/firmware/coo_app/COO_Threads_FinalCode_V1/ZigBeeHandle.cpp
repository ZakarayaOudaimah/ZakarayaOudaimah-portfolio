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

//extern SoftwareSerial Serial2;

TickType_t lastReceiveTime = 0;
TickType_t timingfordeletedScaningDevices = 0;

/*struct alertMessages {
  char messagesOfAlerts[256];
};

struct FailedtMessages {
  char messagesOfFailures[256];
};*/


struct smartLockMessFormat {
  char mess[256];
};


struct eventType {
  char success[8] = "success";
  char failure[8] = "failure";
  char lost[5] = "lost";
};

eventType eventDevice;


struct DeviceMessage {
  char mac[17];
  char message[128];
  char OpenMessage[128];
  TickType_t TimerNode = 0;
  bool sendMAcEndDeviceToBroker = false;
  //bool isNotBelongToThisCoo = false;
  char transactionId[32];
  char userId[32];
};

struct belongNodesToOurCoo {
  char mac[17];
  
};

struct NotbelongNodesToOurCoo {
  char mac[17];
  bool sendTheresponseToNodeThatNotbelong = false;
};
/*struct deletedDevices {
  uint8_t deviceNumber = 0;
  char mac[17];
  bool hasOpenMessageBeforDeleted = false;
  bool sendDeletedMAcEndDeviceToBroker = false;
  char transactionId[32];
  char userId[32];
};*/
uint32_t timeout =  24000;
uint8_t deviceCount = 0;
//uint8_t deletedDeviceCount = 0;
//uint8_t alertCounterOfMessages = 0;
//uint8_t failedCounterOfMessages = 0;
uint8_t belongNodesNumber = 0;
uint8_t notbelongNodesNumber = 0;
uint8_t smartLockMessForLocksNumber = 0;
//uint8_t numDisConnedted = 0;//

DeviceMessage devices[MAX_DEVICES];
belongNodesToOurCoo belongEndDevices[MAX_DEVICES];
NotbelongNodesToOurCoo notbelongEndDevices[MAX_DEVICES];
smartLockMessFormat smartLockMessForLocks[MAX_DEVICES + 20];
//deletedDevices OutDevices[MAX_DEVICES];
//alertMessages alerts[MAX_DEVICES];
//FailedtMessages Failures[MAX_DEVICES];

char buffer[512] = {0};

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
//extern bool messageArrivedOnTopic;
bool sendDatabase = false;



//hardware Check
bool hardwareCheck(char *Mess){
  if (strlen(Mess) > 0){
    if(strncmp(Mess , "Error" , 5) == 0 || strncmp(Mess , "Enter AT Mode" , 13) == 0){
      return true;
    }else{
      Serial.println("Hardware Not detected, please see you hardware....");
      return false;
    }
  }
  else{
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

    char *p_pan   = strstr(Mess, "PANID:");
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

    char panid[5] = {0};
    strncpy(panid, p_pan, 4);

    /* Move pointer to after "EXPANID: " */
    p_expan += strlen("EXPANID: ");

    if (p_expan[0] == '0' && (p_expan[1] == 'x' || p_expan[1] == 'X'))
        p_expan += 2;

    char expanid[17] = {0};
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
  char *p = strstr(Mess , "Mac Addr");
  int counter_index = 0;
  if (p != NULL) {
    while(p[counter_index] != '\0') {
      if(p[counter_index] == ':') {
        counter_index += 4;
        for(int i = 0 ; i < 16 ; i++) {
          MAC_Address[i] = p[counter_index++];
        }
        MAC_Address[16] = '\0';
        break;
      }
      counter_index++;
    }
  }
  if(MAC_Address[0] != '\0') {
    Serial.println("\nMAC address found ... ");
    if (strlen(MAC_Address) != 16) {
      getMAC = false;
      return;
    }
    for (size_t i = 0; i < 16; i++) {
      if (!isxdigit(MAC_Address[i])) {
        getMAC = false;
        return;
      }
    }
    Serial.print("MAC_ZigBee is : ");
    Serial.println(MAC_Address);
    Serial.println();
    ConvertStringMACtoCharr(MAC_Address);
    getMAC = true;
    //Serial.println("GETMAC true... ");
  }
  else{
    Serial.println("\n\nMAC address Not found ... ");
    getMAC = false;
    return;
  }


  
}


void ReschedualBelongDatabase(char *Index){
  for(int i = 0 ; i < belongNodesNumber ; i++){
    if(Index[i] == 0){
      continue;
    }
    else if(Index[i] == 1 && i < belongNodesNumber){
      int y = i + 1;
      if( y == belongNodesNumber){
        belongEndDevices[i].mac[0] = '\0';
        //devices[i].isNotBelongToThisCoo = false;
      }
      for(y ; y < belongNodesNumber ; y++){
        strncpy(belongEndDevices[y-1].mac, belongEndDevices[y].mac, 16);
        belongEndDevices[y].mac[0] = '\0';
      }
      //ThereisAnUpdate = true;
    }
  }
}

bool parseMACsAndDeleting(const char* json) {
  StaticJsonDocument<512> doc;   // حجم كافي للرسالة
  char disconnectedIndex [belongNodesNumber] = {0};

  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.print("❌ JSON Parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  // نتاكد إن في array باسم "mac"
  if (!doc.containsKey("mac")) {
    Serial.println("❌ No mac key found in JSON");
    return false;
  }

  JsonArray arr = doc["mac"].as<JsonArray>();

  for (JsonVariant v : arr) {
    const char* macStr = v.as<const char*>();
    if (macStr == nullptr) continue;

    //bool found = false;
    for (int i = 0; i < belongNodesNumber; i++) {
      if (strncmp(belongEndDevices[i].mac, macStr, 16) == 0) {
        belongEndDevices[i].mac[0] == '\0';
        disconnectedIndex[i] = 1;
        ReschedualBelongDatabase(disconnectedIndex);
        belongNodesNumber--;
        break;
      }
    }
  }

  Serial.print("📊 Stored devices: ");
  Serial.println(belongNodesNumber);
  return true;
}

bool parseMACs(const char* json) {
  StaticJsonDocument<512> doc;   // حجم كافي للرسالة

  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.print("❌ JSON Parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  // نتاكد إن في array باسم "mac"
  if (!doc.containsKey("mac")) {
    Serial.println("❌ No mac key found in JSON");
    return false;
  }

  JsonArray arr = doc["mac"].as<JsonArray>();

  for (JsonVariant v : arr) {
    const char* macStr = v.as<const char*>();
    if (macStr == nullptr) continue;

    bool found = false;
    for (int i = 0; i < belongNodesNumber; i++) {
      if (strncmp(belongEndDevices[i].mac, macStr, 16) == 0) {
        //Serial.print("✅ MAC Found: ");
        //Serial.println(macStr);
        found = true;
        break;
      }
    }

    if (!found) {
      if (belongNodesNumber < MAX_DEVICES) {
        strncpy(belongEndDevices[belongNodesNumber].mac, macStr, 16);
        belongEndDevices[belongNodesNumber].mac[16] = '\0';
        //Serial.print("➕ MAC Added: ");
        //Serial.println(macStr);
        belongNodesNumber++;
      } else {
        Serial.println("⚠️ Max devices reached, cannot add more.");
      }
    }
  }

  Serial.print("📊 Stored devices: ");
  Serial.println(belongNodesNumber);
  return true;
}


void handleOpenDoor(){
  //if(messageArrivedOnTopic == true){
    
  if(deviceCount != 0 && messageArriverOnMACCooTopic[0] != '\0' && !messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes == false){
    bool jsonFormat = checkJSONFormat(messageArriverOnMACCooTopic);
    if(jsonFormat){
      JsonData result = parseJson(messageArriverOnMACCooTopic);
      //memset(messageArriverOnMACCooTopic, '\0', 2048);
      messageArriverOnMACCooTopic[0] = '\0';
      //Serial.println("Parsing json Message");
      /*Serial.println("✅ Parsed Data:");
      Serial.print("MAC: "); Serial.println(result.mac);
      Serial.print("Command: "); Serial.println(result.command);
      Serial.print("User ID: "); Serial.println(result.userId);
      Serial.print("Transaction ID: "); Serial.println(result.transactionId);*/
      bool findaDevice = false;
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(result.mac, devices[i].mac , 16) == 0) {
          //memset(devices[i].OpenMessage , '\0' , 128);
          //memset(devices[i].transactionId , '\0' , 32);
          //memset(devices[i].userId , '\0' , 32);
          devices[i].OpenMessage[0] = '\0';
          devices[i].transactionId[0] = '\0';
          devices[i].userId[0] = '\0';
          devices[i].OpenMessage[0] = '\0';
          strcat(devices[i].OpenMessage, "OP:A, MAC:");
          strcat(devices[i].OpenMessage, result.mac);
          strcat(devices[i].OpenMessage, ", AC:O#");
          strcat(devices[i].transactionId, result.transactionId);
          strcat(devices[i].userId, result.userId);
          //strncpy(devices[i].OpenMessage, result.command , sizeof(devices[i].OpenMessage) - 1);
          devices[i].OpenMessage[sizeof(devices[i].OpenMessage) - 1] = '\0';

          //Serial.print(devices[i].OpenMessage);
          findaDevice = true;
          break;
        }
      }
      /*if(findaDevice){
        Serial.print("\nThere is a device on this MAC\n"); 
      }
      else{
        Serial.print("\nThere is no device on this MAC\n"); 
      }*/
      //messageArrivedOnTopic = false;
      return;
    }
    else{
      messageArriverOnMACCooTopic[0] = '\0';
      //messageArrivedOnTopic = false;
      return;
    }
  }
  else if(messageArriverOnMACCooTopic[0] != '\0' && messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes == false ){
    Serial.print("\n\nHandling DataBaseBelong to nodes belong to our Coo with MAC: ");
    Serial.print(MAC_Address);
    Serial.print("\n\n");
    if (parseMACs(messageArriverOnMACCooTopic)) {
      if(belongNodesNumber > 0){
        Serial.print("\n\nnodes belong to our Coo are: \n");
        for (int i = 0 ; i < belongNodesNumber ; i++ ){
          if(belongEndDevices[i].mac[0] != '\0'){
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
  }
  else if(messageArriverOnMACCooTopic[0] != '\0' && !messageArrivedOnBleongsNodes && messageArrivedOnDeletingOnBleongsNodes){
    Serial.print("\n\nHandling deleting a Node from belongDatabase to nodes belong to our Coo with MAC: ");
    Serial.print(MAC_Address);
    Serial.print("\n\n");
    if (parseMACsAndDeleting(messageArriverOnMACCooTopic)) {
      if(belongNodesNumber > 0){
        Serial.print("\n\nnodes belong to our Coo are: \n");
        for (int i = 0 ; i < belongNodesNumber ; i++ ){
          if(belongEndDevices[i].mac[0] != '\0'){
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
  }
  else{
    messageArriverOnMACCooTopic[0] = '\0';
  }
}


// Send Command
void CommToZigBee( char *Mess , bool checkCooNet , int timing ) {
  response[0] = '\0';
  counter_response = 0;
  bool messageRecieve = false;
  TickType_t lastTimeforResponse = xTaskGetTickCount();
  if(strncmp(Mess, "ATRB", 4) == 0){
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
      if(strstr(response, "OK") != NULL){
        messageRecieve = true;
        Serial.println();
        Serial.println();
      }
      else continue;
    }
  }
  else if(strncmp(Mess, "ATIF", 4) == 0){
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
      if(strstr(response, "OK") != NULL){
        messageRecieve = true;
        
        if( checkCooNet ){
            checkCooNetworkSetting = checkCooNetwork(response);
        }
        else{
            MAC_ZigBee(response);
        }

        Serial.println();
        break;
      }
      else {
        //Serial.print("OK not found on command to Zigbee ATIF");
        continue;
      }
    }
  }
  else{
    RepateSending:
    lastTimeforResponse = xTaskGetTickCount();
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
        //Serial.write(c);  // للطباعة على الشاشة بدون تشويش
        if(c == '\n'){
          if(strncmp(Mess, "+++", 3) == 0){
            if(strncmp(response , "Error" , 5) == 0 || strncmp(response , "Enter AT Mode" , 13) == 0){
              //Serial.print(response);
              messageRecieve = true;
              hardwareZigBee = true;
              Serial.println();
              break;
            }
            else continue;
          }
          else if(strncmp(Mess, "ATTM", 4) == 0){
            if(strstr(response, "OK") != NULL){
              messageRecieve = true;
              Serial.println();
              break;
            }
            else continue;
          }
          else if(strncmp(Mess, "ATDA", 4) == 0){
            if(strstr(response, "OK") != NULL){
              messageRecieve = true;
              Serial.println();
              break;
            }
            else continue;
          }
          else if(strncmp(Mess, "ATPA", 4) == 0){
            if(strstr(response, "OK") != NULL){
              messageRecieve = true;
              Serial.println();
              break;
            }
            else continue;
          }
          else if(strncmp(Mess, "ATDT", 4) == 0){
            if(strstr(response, "OK") != NULL){
              messageRecieve = true;
              Serial.println();
              break;
            }
            else continue;
          }
        }
      }

      if(response[0] == '\0' && ( xTaskGetTickCount() - lastTimeforResponse >= 1000) ){
        Serial.println("There is no hardware please check .... \n");
        hardwareZigBee = false;
        ZigBee_init = false;
        goto RepateSending;
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
  while(!hardwareZigBee){
    CommToZigBee("+++\r\n" , false , 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  
  while(getMAC == false){
    CommToZigBee("ATIF\r\n", false , 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  CommToZigBee("ATPA1\r\n", false , 0);
  vTaskDelay(pdMS_TO_TICKS(3000));
  CommToZigBee("ATRB\r\n", false , 0);
  vTaskDelay(pdMS_TO_TICKS(3000));

  while(1){
    CommToZigBee("ATIF\r\n", true , 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    if( checkCooNetworkSetting ){
        break;
    }
    else{
      CommToZigBee("ATRB\r\n", false , 0);
      vTaskDelay(pdMS_TO_TICKS(10000));
    }

  }

  Serial.println();
  CommToZigBee("ATDT\r\n", false , 0);
  Serial.print("ZigBee_init ");
  Serial.println("true... ");
  ZigBee_init = true;
  response[0] = '\0';
  counter_response = 0;
}


/*void runningEveryTimeUntilAlertOn(){
  if(alertCommand == true && publicAlertReplayFlage){
    if(MQTT_FlageOK && DHCP_FlageOK){
      Serial.print("\n\nSending alert messages \n\n");
      if(alertCounterOfMessages > 0 ){
        for(int i = 0 ; i < 10 ; i++){
          if(alerts[i].messagesOfAlerts[0] != '\0'){
            Serial.print("\n\nSending alert message ");
            Serial.print(i);
            Serial.print("\n");
            Serial.print(alerts[i].messagesOfAlerts);
            Serial.print("\n\n");
            PublishAlertMessages(alerts[i].messagesOfAlerts);
            alerts[i].messagesOfAlerts[0] = '\0';
            alertCounterOfMessages--;
          }
        }
      }
      if(alertCounterOfMessages == 0){
        alertCommand = false;
      }
    }
  }
}*/


/*void runningEveryTimeUntilFailureOn(){
  if(failureCommand == true && publicAlertReplayFlage){
    if(MQTT_FlageOK && DHCP_FlageOK){
      Serial.print("\n\nSending failures messages \n\n");
      if(failedCounterOfMessages > 0 ){
        for(int i = 0 ; i < 10 ; i++){
          if(Failures[i].messagesOfFailures[0] != '\0'){
            Serial.print("\n\nSending failures message ");
            Serial.print(i);
            Serial.print("\n");
            Serial.print(Failures[i].messagesOfFailures);
            Serial.print("\n\n");
            PublishCommandReply(Failures[i].messagesOfFailures);
            Failures[i].messagesOfFailures[0] = '\0';
            failedCounterOfMessages--;
          }
        }
      }
      if(failedCounterOfMessages == 0){
        failureCommand = false;
      }
    }
  }
}*/


/*void commandReplyForming(uint8_t indexOfDevice){
  if(aNodeDiscoonected == false){
    if(MQTT_FlageOK && DHCP_FlageOK){
      StaticJsonDocument<256> doc;
      // إعادة ملء JSON بالبيانات من struct
      doc["event"] = "command_success";
      doc["eventType"] = eventDevice.success;
      doc["transactionId"] = devices[indexOfDevice].transactionId;
      doc["deviceId"] = devices[indexOfDevice].mac;
      doc["message"] = "door unlocked successfully";
      doc["userId"] = devices[indexOfDevice].userId;
      serializeJsonPretty(doc, Serial);
      // Create a character array to hold the serialized JSON string
      char jsonBuffer[256];

      // Serialize the JSON document into the buffer
      serializeJson(doc, jsonBuffer);
      PublishCommandReply(jsonBuffer);
    }
    else if(!MQTT_FlageOK || !DHCP_FlageOK){
      alertCommand = true;
      Serial.print("\n\nreshape alert message \n\n");
      StaticJsonDocument<256> doc;
      // إعادة ملء JSON بالبيانات من struct
      doc["event"] = "delayed_success";
      doc["eventType"] = eventDevice.success;
      doc["transactionId"] = devices[indexOfDevice].transactionId;
      doc["deviceId"] = devices[indexOfDevice].mac;
      doc["message"] = "door unlocked successfully after reconnection";
      doc["userId"] = devices[indexOfDevice].userId;
      for(int z = 0 ; z < 10 ; z++ ){
        if(alerts[z].messagesOfAlerts[0] == '\0'){
          serializeJson(doc, alerts[z].messagesOfAlerts);
          alertCounterOfMessages++;
          break;
        }
      }
    }
  }
  else{
    if(MQTT_FlageOK && DHCP_FlageOK){
      StaticJsonDocument<256> doc;
      // إعادة ملء JSON بالبيانات من struct
      doc["event"] = "connection_failure";
      doc["eventType"] = eventDevice.failure;
      //doc["transactionId"] = devices[indexOfDevice].transactionId;
      doc["deviceId"] = devices[indexOfDevice].mac;
      doc["message"] = "This node is disconnected from a network";
      char jsonBuffer[256];

      // Serialize the JSON document into the buffer
      serializeJson(doc, jsonBuffer);
      PublishAlertMessages(jsonBuffer);
    }
    else if(!MQTT_FlageOK || !DHCP_FlageOK){
      alertCommand = true;
      Serial.print("\n\nreshape alert message \n\n");
      StaticJsonDocument<256> doc;
      // إعادة ملء JSON بالبيانات من struct
      doc["event"] = "connection_failure";
      doc["eventType"] = eventDevice.failure;
      //doc["transactionId"] = devices[indexOfDevice].transactionId;
      doc["deviceId"] = devices[indexOfDevice].mac;
      doc["message"] = "This node is disconnected from a network";
      //doc["userId"] = devices[indexOfDevice].userId;
      for(int z = 0 ; z < 10 ; z++ ){
        if(alerts[z].messagesOfAlerts[0] == '\0'){
          serializeJson(doc, alerts[z].messagesOfAlerts);
          alertCounterOfMessages++;
          break;
        }
      }
    }
  }
}*/


/*void commandReplyFailure(char *mac , uint8_t indexOfDevice){
  if(MQTT_FlageOK && DHCP_FlageOK && failureCommand == false){
    StaticJsonDocument<256> doc;
    // إعادة ملء JSON بالبيانات من struct
    doc["event"] = "command_failed";
    doc["eventType"] = eventDevice.failure;
    doc["transactionId"] = devices[indexOfDevice].transactionId;
    doc["deviceId"] = devices[indexOfDevice].mac;
    doc["message"] = "door didn't open successfully";
    doc["userId"] = devices[indexOfDevice].userId;
    serializeJsonPretty(doc, Serial);
    // Create a character array to hold the serialized JSON string
    char jsonBuffer[256];

    // Serialize the JSON document into the buffer
    serializeJson(doc, jsonBuffer);
    PublishCommandReply(jsonBuffer);
  }
  else if(!MQTT_FlageOK || !DHCP_FlageOK){
    Serial.print("\n\nreshape Failed message \n\n");
    failureCommand = true;
    StaticJsonDocument<256> doc;
    // إعادة ملء JSON بالبيانات من struct
    doc["event"] = "command_failed";
    doc["eventType"] = eventDevice.failure;
    doc["transactionId"] = devices[indexOfDevice].transactionId;
    doc["deviceId"] = devices[indexOfDevice].mac;
    doc["message"] = "door didn't open successfully";
    doc["userId"] = devices[indexOfDevice].userId;
    for(int z = 0 ; z < 10 ; z++ ){
      if(Failures[z].messagesOfFailures[0] == '\0'){
        serializeJson(doc, Failures[z].messagesOfFailures);
        failedCounterOfMessages++;
        break;
      }
    }
  }
}*/

/*void AlertOnNotBelongDataWhenDisconnecting( uint8_t indexOfDevice ){
  if(MQTT_FlageOK && DHCP_FlageOK){
    StaticJsonDocument<256> doc;
    // إعادة ملء JSON بالبيانات من struct
    doc["event"] = "connection_failure";
    doc["eventType"] = eventDevice.failure;
    //doc["transactionId"] = devices[indexOfDevice].transactionId;
    doc["deviceId"] = notbelongEndDevices[indexOfDevice].mac;
    doc["message"] = "This node is disconnected from a network";
    char jsonBuffer[256];

    // Serialize the JSON document into the buffer
    serializeJson(doc, jsonBuffer);
    PublishAlertMessages(jsonBuffer);
  }
  else if(!MQTT_FlageOK || !DHCP_FlageOK){
    alertCommand = true;
    Serial.print("\n\nreshape alert message \n\n");
    StaticJsonDocument<256> doc;
    // إعادة ملء JSON بالبيانات من struct
    doc["event"] = "connection_failure";
    doc["eventType"] = eventDevice.failure;
    //doc["transactionId"] = devices[indexOfDevice].transactionId;
    doc["deviceId"] = notbelongEndDevices[indexOfDevice].mac;
    doc["message"] = "This node is disconnected from a network";
    //doc["userId"] = devices[indexOfDevice].userId;
    for(int z = 0 ; z < 10 ; z++ ){
      if(alerts[z].messagesOfAlerts[0] == '\0'){
        serializeJson(doc, alerts[z].messagesOfAlerts);
        alertCounterOfMessages++;
        break;
      }
    }
  }
}*/


void smartLockMessage( char *mac , uint8_t indexOfNode ,  bool disconnected , bool openDoorFailure , bool batteryCheck , bool openDoorSuccess ){

    StaticJsonDocument<256> doc;
    

    if( MQTT_FlageOK && DHCP_FlageOK ){

        if( disconnected ){
          doc["level"] = eventDevice.failure;
          doc["mac"] = devices[indexOfNode].mac;
          doc["message"] = "This node is disconnected from a network";
        }
        else if( openDoorFailure ){
          doc["level"] = eventDevice.failure;
          doc["transactionId"] = devices[indexOfNode].transactionId;
          doc["mac"] = devices[indexOfNode].mac;
          doc["message"] = "door didn't open successfully";
          doc["userId"] = devices[indexOfNode].userId;
        }
        else if( batteryCheck ){

        }
        else if ( openDoorSuccess ){
          doc["level"] = eventDevice.success;
          doc["transactionId"] = devices[indexOfNode].transactionId;
          doc["mac"] = devices[indexOfNode].mac;
          doc["message"] = "door unlocked successfully";
          doc["userId"] = devices[indexOfNode].userId;
        }


        // Create a character array to hold the serialized JSON string
        char jsonBuffer[256];

        // Serialize the JSON document into the buffer
        serializeJson(doc, jsonBuffer);
        publishDevices( "smartLock" ,  jsonBuffer );
    }
    else if( !MQTT_FlageOK || !DHCP_FlageOK ){

        if( disconnected ){

        }
        else if( openDoorFailure ){

        }
        else if( batteryCheck ){

        }
        else if ( openDoorSuccess ){

        }

    }

}

void ReschedualNotBelongDatabase(char *Index){
  for(int i = 0 ; i < notbelongNodesNumber ; i++){
    if(Index[i] == 0){
      continue;
    }
    else if(Index[i] == 1 && i < notbelongNodesNumber){
      int y = i + 1;
      if( y == notbelongNodesNumber){
        notbelongEndDevices[i].mac[0] = '\0';
        notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong = false;
        //devices[i].isNotBelongToThisCoo = false;
      }
      for(y ; y < notbelongNodesNumber ; y++){
        strncpy(notbelongEndDevices[y-1].mac, notbelongEndDevices[y].mac, 16);
        notbelongEndDevices[y].mac[0] = '\0';
        notbelongEndDevices[y-1].sendTheresponseToNodeThatNotbelong = notbelongEndDevices[y].sendTheresponseToNodeThatNotbelong;
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
    strncpy(line, buffer, lineLength);
    line[lineLength] = '\0';

    // Remove processed line from buffer
    memmove(buffer, newlinePtr + 1, strlen(newlinePtr + 1) + 1);

    // Remove trailing '\r' or space
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ')) {
      line[--len] = '\0';
    }

    // === Handle Format 1: OP:N, MA:<MAC> ===
    if (strncmp(line, "OP:N, MAC:", 10) == 0) {
      char mac[17];
      strncpy(mac, line + 10, 16);
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

      bool found = false;
      for (int i = 0; i < deviceCount ; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0 && deviceCount > 0) {
          strncpy(devices[i].message, line, sizeof(devices[i].message) - 1);
          devices[i].message[sizeof(devices[i].message) - 1] = '\0';
          //Serial.println(devices[i].message);
          strcat(responseBuffer, line);//, sizeof(devices[i].message) - 1);
          strcat(responseBuffer, ", AC:S\n");
          devices[i].TimerNode = xTaskGetTickCount();
          Serial.print(responseBuffer);
          Serial2.print(responseBuffer);
          responseBuffer[0] = '\0';
          if(devices[i].OpenMessage[0] != '\0' && devices[i].sendMAcEndDeviceToBroker == true){
            responseBuffer[0] = '\0';
            Serial.print("\n\nThere is open message for device: ");
            Serial.print(i);
            Serial.print(" ,MAC: ");
            Serial.print(devices[i].mac);
            Serial.print(" ,Message is: Failure");
            //test
            //commandReplyFailure(devices[i].mac , i );
            //test
            devices[i].OpenMessage[0] = '\0';
          }
          found = true;
          break;
        }
      }

      if (!found && deviceCount < MAX_DEVICES && belongNodesNumber > 0) {
        bool foundInOurBelongNodes = false;
        for(int i = 0 ; i < belongNodesNumber ; i++){
          if(belongEndDevices[i].mac[0] != '\0'){
            if(strncmp(belongEndDevices[i].mac, mac, 16) == 0){
              //Serial.print("\n\nThis node is belonge to me with MAC:");
              //Serial.println(mac);
              foundInOurBelongNodes = true;
              break;
            }
          }
        }
        
        if(foundInOurBelongNodes){
          
          strncpy(devices[deviceCount].mac, mac, 16);
          devices[deviceCount].mac[16] = '\0';
          devices[deviceCount].TimerNode = xTaskGetTickCount();
          strncpy(devices[deviceCount].message, line, sizeof(devices[deviceCount].message) - 1);
          devices[deviceCount].message[sizeof(devices[deviceCount].message) - 1] = '\0';

          strcat(responseBuffer, devices[deviceCount].message);
          strcat(responseBuffer, ", AC:S\n");
          //Serial.print(responseBuffer);
          deviceCount = deviceCount + 1;
        }
        else{
          Serial.print("\n\nWait you Node To send you to the Broker,\nand resend the response to what MAC Coo you belonge to...\n\n");
        }
      }
    }
    else if (strncmp(line, "OP:S, MAC:", 10) == 0){
      //Serial.println(line);
      char mac[17];
      strncpy(mac, line + 10, 16);
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
      for(int z = 0; z < notbelongNodesNumber ; z++){
        if(strncmp(notbelongEndDevices[z].mac , mac , 16) == 0){
          Serial.print("\n\nyou are now not belong to me\n\n");
          notbelongEndDevices[z].sendTheresponseToNodeThatNotbelong = true;
          //notbelongEndDevices[z].mac[0] = '\0';
          break;
        }
      }
      for (int i = 0; i < deviceCount; i++) {
        if (strncmp(devices[i].mac, mac, 16) == 0) {
          Serial.print("Add new Time for device: ");
          Serial.print(i);
          Serial.print(" ,MAC: ");
          Serial.print(devices[i].mac);
          Serial.print(" ");
          devices[i].TimerNode = xTaskGetTickCount();
          Serial.print(" ");
          Serial.println(devices[i].TimerNode);
          if(devices[i].OpenMessage[0] != '\0'){
            Serial.print("There is open message for device: ");
            Serial.print(i);
            Serial.print(" ,MAC: ");
            Serial.print(devices[i].mac);
            Serial.print(" ,Message is: ");
            Serial.println(devices[i].OpenMessage);
            Serial2.println(devices[i].OpenMessage);
            //test
            //commandReplyForming(i);
            //test
            devices[i].OpenMessage[0] = '\0';
          }
          break;
        }
      }
    }
    else if(strncmp(line, "OP:A, MA:", 9) == 0){
      //Serial.println(line);
      char mac[17];
      strncpy(mac, line + 9, 16);
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
        if (strncmp(devices[i].mac, mac, 16) == 0) {
          Serial.print("Recieved OK form device: ");
          Serial.print(i);
          Serial.print(" ,MAC: ");
          Serial.println(devices[i].mac);
          devices[i].OpenMessage[0] = '\0';
        }
      }
    }
    else{
       buffer[0] = '\0';
    }
  }

  if (strlen(responseBuffer) > 0) {
    Serial.print(responseBuffer);
    Serial2.print(responseBuffer);
  }

  // Print device list every 10s
  if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(10000)) {

    //Test
    /*devices[1].TimerNode = millis();
    Serial.print(devices[1].mac);
    Serial.print(" ");
    Serial.println(devices[1].TimerNode);*/
    //Test


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
        Serial.println("]");
      }
      Serial.println();
    }
    lastReceiveTime = xTaskGetTickCount();
  }
}

void ReschedualDevices(char *Index){
  for(int i = 0 ; i < deviceCount ; i++){
    if(Index[i] == 0){
      continue;
    }
    else if(Index[i] == 1 && i < deviceCount){
      int y = i + 1;
      if( y == deviceCount){
        devices[i].mac[0] = '\0';
        devices[i].message[0] = '\0';
        devices[i].OpenMessage[0] = '\0';
        devices[i].TimerNode = 0;
        devices[i].transactionId[0] = '\0';
        devices[i].userId[0] = '\0';
        devices[i].sendMAcEndDeviceToBroker = false;
        //devices[i].isNotBelongToThisCoo = false;
        return;
      }
      for(y ; y < deviceCount ; y++){
        strncpy(devices[y-1].mac, devices[y].mac, 16);
        devices[y].mac[0] = '\0';
        strncpy(devices[y-1].message, devices[y].message, sizeof(devices[y].message) - 1);
        devices[y].message[0] = '\0';
        strncpy(devices[y-1].OpenMessage, devices[y].OpenMessage, sizeof(devices[y].OpenMessage) - 1);
        devices[y].OpenMessage[0] = '\0';
        devices[y-1].TimerNode = devices[y].TimerNode;
        devices[y].TimerNode = 0;
        devices[y-1].sendMAcEndDeviceToBroker = devices[y].sendMAcEndDeviceToBroker;
        devices[y].sendMAcEndDeviceToBroker = false;
        //devices[i].isNotBelongToThisCoo = devices[y].isNotBelongToThisCoo;
        //devices[y].isNotBelongToThisCoo = false;
        strncpy(devices[y-1].transactionId, devices[y].transactionId, sizeof(devices[y].transactionId) - 1);
        devices[y].transactionId[0] = '\0';
        strncpy(devices[y-1].userId, devices[y].userId, sizeof(devices[y].userId) - 1);
        devices[y].userId[0] = '\0';
      }
      //ThereisAnUpdate = true;
    }
  }
  //ThereisAnUpdate = true;
}



void UpdateNetwork(int enddevicenum , char *mac){
  StaticJsonDocument<256> doc;
  // Create a character array to hold the serialized JSON string
  char jsonBuffer[256];

  char number[3];
  itoa(enddevicenum , number,  10);
  doc["device"] = number;


  doc["mac"] = mac;
  doc["ac"] = "d";

  // Serialize the JSON document into the buffer
  serializeJson(doc, jsonBuffer);

  PublishUpdateOnNetwork(jsonBuffer);
}

void addNodeToNotbelongeDatabase(char* mac){
  bool found = false;
  for(int i = 0 ; i < notbelongNodesNumber ; i++ ){
    if(strncmp(notbelongEndDevices[i].mac, mac , 16) == 0){
      found = true;
      break;
    }
  }
  if(found){
    return;
  }
  else{
    strncpy(notbelongEndDevices[notbelongNodesNumber].mac , mac , 16);
    notbelongNodesNumber++;
    Serial.print("\n\nadded To our database for not belonge\n\n");
  }
  for(int i = 0 ; i < notbelongNodesNumber ; i++ ){
    Serial.print("\n\nNot Belong data: ");
    Serial.print(notbelongNodesNumber);
    Serial.print("\n\n");
    Serial.print(i);
    Serial.print(", ");
    Serial.println(notbelongEndDevices[i].mac);
  }
  Serial.print("\n\n");
}

void TimersForNodes(){
  if(notbelongNodesNumber > 0){
    char disconnectedIndex [notbelongNodesNumber] = {0};
    for(int i = 0 ; i < notbelongNodesNumber ; i++){
      if(notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong == true){
        //aNodeDiscoonected = true;
        Serial.print("\n\nWe must send it to broker now\n\n");
        //AlertOnNotBelongDataWhenDisconnecting(i);
        //aNodeDiscoonected = false;
        notbelongEndDevices[i].sendTheresponseToNodeThatNotbelong = false;
        notbelongEndDevices[i].mac[0] = '\0';
        disconnectedIndex[i] = 1;
        ReschedualNotBelongDatabase(disconnectedIndex);
        notbelongNodesNumber--;
        break;
      }
    }
  }
  if(belongNodesNumber > 0){
    char disconnectedIndex [deviceCount] = {0};
    for (int i = 0 ; i < deviceCount ; i++){
      bool found = false;
      for (int z = 0 ; z < belongNodesNumber ; z++){
        if(devices[i].mac[0] != '\0'){
          if(strncmp(belongEndDevices[z].mac, devices[i].mac , 16) == 0){
            found = true;
            break;
          }
        }
      }
      if(!found){
        //devices[i].isNotBelongToThisCoo = true;
        Serial.print("\nwe finde a node is now not belonging to our COO with its MAC: ");
        Serial.print("Device ");
        Serial.print(i);
        Serial.print(" [");
        Serial.print(devices[i].mac);
        Serial.print("]  ");
        if(devices[i].OpenMessage[0] != '\0'){
          //commandReplyFailure(devices[i].mac, i);
        }
        /*if(MQTT_FlageOK){
          UpdateNetwork(i , devices[i].mac );
        }*/
        /*aNodeDiscoonected = true;
        commandReplyForming(i);
        aNodeDiscoonected = false;
        if(devices[i].OpenMessage[0] != '\0'){
          commandReplyFailure(devices[i].mac, i);
        }*/
        addNodeToNotbelongeDatabase(devices[i].mac);
        devices[i].mac[0] = '\0';
        devices[i].message[0] = '\0';
        devices[i].OpenMessage[0] = '\0';
        devices[i].transactionId[0] = '\0';
        devices[i].userId[0] = '\0';
        devices[i].TimerNode = 0;
        devices[i].sendMAcEndDeviceToBroker = false;
        //devices[i].isNotBelongToThisCoo = false;
        disconnectedIndex [i] = 1;
        ReschedualDevices(disconnectedIndex);
        deviceCount = deviceCount - 1;
        break;
      }
    }
  }
  if(deviceCount != 0){
    char disconnectedIndex [deviceCount] = {0};
    //bool Reschedual = false;
    for (int i = 0 ; i < deviceCount ; i++){
      if(strlen(devices[i].mac) > 0){
        if( ( xTaskGetTickCount() - devices[i].TimerNode ) >= pdMS_TO_TICKS(timeout) ){
          //Serial.println( xTaskGetTickCount() - devices[i].TimerNode);
          Serial.print("Device ");
          Serial.print(i);
          Serial.print(" [");
          Serial.print(devices[i].mac);
          Serial.print("]  ");
          Serial.println("Disconnected");
          
          smartLockMessage( devices[i].mac , i , true , false , false , false );


          devices[i].mac[0] = '\0';
          devices[i].message[0] = '\0';
          devices[i].OpenMessage[0] = '\0';
          devices[i].transactionId[0] = '\0';
          devices[i].userId[0] = '\0';
          devices[i].TimerNode = 0;
          devices[i].sendMAcEndDeviceToBroker = false;
          //devices[i].isNotBelongToThisCoo = false;
          disconnectedIndex [i] = 1;
          ReschedualDevices(disconnectedIndex);
          deviceCount = deviceCount - 1;
          break;
        }
      }
    }
  }
}

void sendDataBASE_AfterConnectionIsDone(){
  StaticJsonDocument<256> doc;
  // Create a character array to hold the serialized JSON string
  char jsonBufferUpdate[512];

  // نعمل مصفوفة تحت اسم "database"
  JsonArray mac = doc.createNestedArray("mac");

  if (deviceCount == 0) {
    //doc["mac"] = mac.add("");//NULL;
  } 
  else {
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

void SendNewMacEndDevice(){
  StaticJsonDocument<256> doc;
  if(deviceCount > 0){
    for (int i = 0; i < deviceCount; i++) {
      if(devices[i].sendMAcEndDeviceToBroker == false){

        char number[3];
        itoa(i, number,  10);
        doc["device"] = number;


        // Create a character array to hold the serialized JSON string
        char jsonBuffer[256];
        doc["mac"] = devices[i].mac;

        // Serialize the JSON document into the buffer
        serializeJson(doc, jsonBuffer);
        if (MQTT_FlageOK){
          PublishOnNewNodeAdded(jsonBuffer);
          devices[i].sendMAcEndDeviceToBroker = true;
        }
      }
      /*Serial.print("Device ");
      Serial.print(i);
      Serial.print(" [");
      Serial.print(devices[i].mac);
      Serial.println("]");*/
    }
  }
}

extern bool SendLogMACCooToBroker;
//bool weWereHavingBelongDataOnce = false;
#define LED_PIN PB0

void indicators(){
  
  if(ZigBee_init == true && !MQTT_FlageOK && !DHCP_FlageOK){
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(PB1, LOW);
    vTaskDelay(250);
  }
  else if(ZigBee_init == true && !MQTT_FlageOK && DHCP_FlageOK){
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(PB1, HIGH);
    vTaskDelay(250);
  }
  else if(ZigBee_init == true && MQTT_FlageOK && DHCP_FlageOK && belongNodesNumber > 0){// && MQTT_FlageOK && DHCP_FlageOK){
    //Serial.println(belongNodesNumber);
    //weWereHavingBelongDataOnce = true;
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(250);
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(250);
  }
  else if(ZigBee_init == true && MQTT_FlageOK && DHCP_FlageOK && belongNodesNumber == 0){// && weWereHavingBelongDataOnce){// && MQTT_FlageOK && DHCP_FlageOK){
    //Serial.println(belongNodesNumber);
    //SendLogMACCooToBroker = true;
    /*if(weWereHavingBelongDataOnce){
      SendLogMACCooToBroker = false;
      weWereHavingBelongDataOnce = false;
    }*/
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(250);
  }
  else if(!ZigBee_init && !MQTT_FlageOK && !DHCP_FlageOK){
    digitalWrite(LED_PIN, LOW);
    digitalWrite(PB1, LOW);
    vTaskDelay(250);
  }
}