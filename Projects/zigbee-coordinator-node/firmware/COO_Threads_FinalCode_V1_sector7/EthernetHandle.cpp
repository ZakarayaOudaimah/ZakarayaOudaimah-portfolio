//#include "wiring_digital.h"
#include <EthernetHandle.h>
#include <Arduino.h>



#include "WSerial.h"
#include <NTPClient_Generic.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <EthernetENC.h>
#include <SSLClient.h>
#include "trust_anchors.h"
#include <MQTTPubSubClient_Generic.h>


#include <sys/_stdint.h>
#include <stdint.h>
#include "wiring_time.h"
#include <string.h>
#include <ArduinoJson.h>

#include <cstdint>

#define MYIPADDR 192, 168, 1, 28
#define MYIPMASK 255, 255, 255, 0
#define MYDNS 192, 168, 1, 1
#define MYGW 192, 168, 1, 1
#define PublishTopicLOG "m@dernl0ck/databse-inquiry/"
//#define PublishDatabase "m@dernl0ck/coos/log"
//#define SubscribeTopicOPENDoor "ModernLock_Door_Action"

char server[] = "dace514c6516469bae194ca09c8c889a.s1.eu.hivemq.cloud";
#define MQTT_PORT 8883
const int rand_pin = A0;
EthernetClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, rand_pin);

void MQTTcallback(char *topic, byte *payload, unsigned int length);
PubSubClient MQTT(server, MQTT_PORT, MQTTcallback, client);

bool getByteMAC = false;
byte ByteMAC[8];
//byte ByteMAC[9] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//byte ByteMAC[8] = { 0x00, 0x15, 0x8D, 0x00, 0x09, 0x9A , 0xB3 , 0x5A };

//TickType_t timingforScaningIsnotOkay = 0;
bool checkENCHardware_Exsiting = false;
bool checkENCCable_Exsiting = false;
bool DHCP_FlageOK = false;
bool MQTT_FlageOK = false;
bool SendLogMACCooToBroker = false;
//bool messageArrivedOnTopic = false;
extern bool messageArrivedOnBleongsNodes;
extern bool messageArrivedOnDeletingOnBleongsNodes;
extern bool commandTome;
extern bool sendDatabase;
bool publicAlertReplayFlage = false;



extern char MAC_Address[17];
char messageArriverOnMACCooTopic[512];


struct domains {
  char smartLock[10] = "smartlock";
};

domains domainType;



void MQTTcallback(char *topic, byte *payload, unsigned int length) {
  /*Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  //int i;
  // Copy payload safely into messageTemp
  for (i = 0; i < length && i < 511; i++) {
    messageArriverOnMACCooTopic[i] = (char)payload[i];
    Serial.print(messageArriverOnMACCooTopic[i]);
  }
  strcat(messageArriverOnMACCooTopic, payload);
  //strncpy(responseBuffer, line, sizeof(payload) - 1);*/
  //if(messageArrivedOnTopic == false){
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  if ((strstr(topic, "connection-joining") != NULL) && (strstr(topic, MAC_Address) != NULL)) {
    messageArrivedOnBleongsNodes = true;
  } else {
    messageArrivedOnBleongsNodes = false;
  }

  if ((strstr(topic, "connection-deleting") != NULL) && (strstr(topic, MAC_Address) != NULL)) {
    messageArrivedOnDeletingOnBleongsNodes = true;
  } else {
    messageArrivedOnDeletingOnBleongsNodes = false;
  }

  if ((strstr(topic, "command") != NULL) && (strstr(topic, MAC_Address) != NULL)) {
    commandTome = true;
  } else {
    commandTome = false;
  }

  // تأكد إن البفر عندك كبير كفاية
  if (length >= sizeof(messageArriverOnMACCooTopic)) {
    Serial.println("❌ Payload too large!");
    return;
  }
  int i = 0;
  // Copy payload safely into messageTemp
  for (i = 0; i < length && i < 512; i++) {
    messageArriverOnMACCooTopic[i] = (char)payload[i];
    //Serial.print(messageArriverOnMACCooTopic[i]);
  }
  // نسخ البايتات إلى البفر
  //strncyp(messageArriverOnMACCooTopic, payload, length);
  messageArriverOnMACCooTopic[length] = '\0';  // إضافة نهاية string
  //Serial.println(messageArriverOnMACCooTopic);
  Serial.println();
  //messageArrivedOnTopic = true;
  //}
}

void ConvertStringMACtoCharr(const char *macString) {
  Ethernet.init(PA4);
  Serial.print("MAC Bytes: ");
  for (int i = 0; i < 8; i++) {
    char byteStr[3] = { macString[i * 2], macString[i * 2 + 1], '\0' };
    ByteMAC[i] = (byte)strtol(byteStr, NULL, 16);
    if (ByteMAC[i] < 0x10) Serial.print("0");  // Add leading zero for single-digit hex
    Serial.print(ByteMAC[i], HEX);
    if (i < 7) Serial.print(":");
  }
  Serial.print("\n");
  getByteMAC = true;
  //Ethernet.begin(ByteMAC);
}


void PublishMACCooRoBroker(char *mess, size_t length) {
  StaticJsonDocument<256> doc;
  if (SendLogMACCooToBroker == false && MQTT_FlageOK) {

    char ModernLock[128] = "m@dernl0ck/databse-inquiry/";
    char COOTopics[128] = { 0 };
    strcat(COOTopics, ModernLock);
    strcat(COOTopics, MAC_Address);
    Serial.print("\nPublish the MAC to Brocker on databse-inquiry Topic : ");
    Serial.print(COOTopics);
    Serial.print(", Message : ");
    Serial.print(mess);
    Serial.println("\n");
    MQTT.publish(COOTopics, mess , length);
    SendLogMACCooToBroker = true;
  }
}

void PublishDataBase(char *database) {
  char ModernLock[50] = "m@dernl0ck/database/";
  char COOTopics[128] = { 0 };
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nPublish the MAC to Brocker on Database Topic : ");
  Serial.print(COOTopics);
  Serial.print(", Message : ");
  Serial.print(database);
  Serial.println("\n");
  MQTT.publish(COOTopics, database);
}


void PublishOnNewNodeAdded(char *MacNewNode, size_t length) {
  char COOTopics[128] = "m@dernl0ck/connection-joining-reply/";
  strcat(COOTopics, MAC_Address);
  MQTT.publish(COOTopics, MacNewNode, length);
}

void publishDevices(char *domain, char *mess, int length) {
  char COOTopics[128] = "m@dernl0ck/devices/";
  if (strncmp(domain, "smartlock", 9) == 0) {
    strcat(COOTopics, domainType.smartLock);
    MQTT.publish(COOTopics, mess, length);
  }
}

void SubsecribeOnCommandTopic() {
  char ModernLock[50] = "m@dernl0ck/command/";
  char COOTopics[128] = { 0 };
  // add new node
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on command Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe(COOTopics);
}


void SubsecribeOnBelongNodesTopic() {
  char ModernLock[50] = "m@dernl0ck/connection-joining/";
  char COOTopics[128] = { 0 };
  // add new node
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on BelongNodes Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe(COOTopics);
}

void SubsecribeOnDeletingBelongNodesTopic() {
  char ModernLock[50] = "m@dernl0ck/connection-deleting/";
  char COOTopics[128] = { 0 };
  // add new node
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on deleting from BelongNodes Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe(COOTopics);
}

void MQTT_ConnectProccessing() {
  // Don't even try if Ethernet isn't ready
  if (!DHCP_FlageOK) {
    return;
  }
  
  // Set buffer size once
  static bool bufferSet = false;
  if (!bufferSet) {
    MQTT.setBufferSize(1024);
    MQTT.setKeepAlive(60); // 120 seconds keepalive
    bufferSet = true;
  }
  
  if (!MQTT.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    // CRITICAL: Give Ethernet time to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Try to connect
    if (MQTT.connect(MAC_Address, "zakaraya", "oudaimah")) {
      vTaskDelay(pdMS_TO_TICKS(500));  // Wait for connection to stabilize
      
      Serial.println("✅ MQTT Connected!");
      
      // Subscribe to topics
      SubsecribeOnCommandTopic();
      SubsecribeOnBelongNodesTopic();
      SubsecribeOnDeletingBelongNodesTopic();
      
      MQTT_FlageOK = true;
      publicAlertReplayFlage = true;
      
    } else {
      Serial.print("❌ MQTT connection failed, rc=");
      Serial.println(MQTT.state());
      
      // Don't retry immediately
      vTaskDelay(pdMS_TO_TICKS(5000));
      MQTT_FlageOK = false;
    }
  }
}


bool checkENCHardwareExsiting() {
  Ethernet.init(PA4);  // CS pin تبع ENC28J60

  // جرب تقرأ حالة الهاردوير
  EthernetHardwareStatus hw = Ethernet.hardwareStatus();
  //Ethernet.init(PA4);
  //Ethernet.end();
  Ethernet.init(PA4);
  if (hw == EthernetNoHardware) {
    Serial.print("Ethernet shield was not found.  Sorry, can't run without hardware. :( ");
    //Serial.print(hw);
    Serial.print("\n");
    digitalWrite(PC13, HIGH);
    vTaskDelay(500);
    digitalWrite(PC13, LOW);
    vTaskDelay(500);
    return false;
  } else {
    Serial.print("Ethernet shield was found. ENC28J60. :( ");
    //Serial.print(hw);
    Serial.print("\n");
    return true;
  }
}

bool checkENCCableExsiting() {
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    //Serial.println("Return To Connect to Ethernet, Please Connect the Cable RJ45.");
    digitalWrite(PC13, HIGH);
    vTaskDelay(250);
    digitalWrite(PC13, LOW);
    vTaskDelay(250);
    digitalWrite(PC13, HIGH);
    vTaskDelay(250);
    digitalWrite(PC13, LOW);
    vTaskDelay(250);
    return false;
  } else if (Ethernet.linkStatus() == LinkON) {
    Serial.println("Ethernet Cable was found. ENC28J60");
    return true;
  }
}
void ENC_Connect() {
  static uint32_t lastAttempt = 0;
  uint32_t now = millis();
  
  // Don't retry more than once every 2 seconds
  if (now - lastAttempt < 2000) {
    return;
  }
  lastAttempt = now;
  
  // Check hardware
  if (!checkENCHardware_Exsiting) {
    checkENCHardware_Exsiting = checkENCHardwareExsiting();
    if (!checkENCHardware_Exsiting) {
      return;
    }
  }

  // Check cable
  if (checkENCHardware_Exsiting && !checkENCCable_Exsiting) {
    checkENCCable_Exsiting = checkENCCableExsiting();
    if (!checkENCCable_Exsiting) {
      return;
    }
  }

  // Try DHCP
  if (checkENCHardware_Exsiting && checkENCCable_Exsiting && !DHCP_FlageOK) {
    Serial.println("\n\n📡 Attempting DHCP connection...");
    
    if (Ethernet.begin(ByteMAC)) {
      Serial.println("✅ DHCP OK!");
      digitalWrite(PB1, HIGH);
      DHCP_FlageOK = true;
      
      // Print network info
      Serial.print("Local IP : ");
      Serial.println(Ethernet.localIP());
      Serial.print("Subnet Mask : ");
      Serial.println(Ethernet.subnetMask());
      Serial.print("Gateway IP : ");
      Serial.println(Ethernet.gatewayIP());
      
      // Give network stack time to initialize
      vTaskDelay(pdMS_TO_TICKS(2000));
      
    } else {
      Serial.println("⚠️ DHCP failed, using static IP");
      IPAddress ip(MYIPADDR);
      IPAddress dns(MYDNS);
      IPAddress gw(MYGW);
      IPAddress sn(MYIPMASK);
      Ethernet.begin(ByteMAC, ip, dns, gw, sn);
      DHCP_FlageOK = true;
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}


// void scaningConnection() {
//   static uint32_t lastLinkCheck = 0;
//   uint32_t now = millis();
  
//   // Check link status every 5 seconds
//   if (now - lastLinkCheck > 5000) {
    
//     // Check cable
//     if (Ethernet.linkStatus() == LinkOFF) {
//       if (checkENCCable_Exsiting == true) {
//         Serial.println("⚠️ Ethernet cable disconnected!");
//         checkENCCable_Exsiting = false;
//       }
      
//       // Reset connection flags
//       DHCP_FlageOK = false;
//       MQTT_FlageOK = false;
//       sendDatabase = false;
//       publicAlertReplayFlage = false;
//       digitalWrite(PB1, LOW);
      
//     } else if (Ethernet.linkStatus() == LinkON) {
//       if (checkENCCable_Exsiting == false) {
//         // Serial.println("✅ Ethernet cable connected!");
//         checkENCCable_Exsiting = true;
        
//         // Force DHCP renewal
//         DHCP_FlageOK = false;
//       }
//     }
    
//     // Check if MQTT disconnected
//     if (MQTT_FlageOK && !MQTT.connected()) {
//       Serial.println("⚠️ MQTT disconnected!");
//       MQTT_FlageOK = false;
//       sendDatabase = false;
//       publicAlertReplayFlage = false;
//     }
    
//     lastLinkCheck = now;
//   }
// }

void scaningConnection(){
  Ethernet.init(PA4);
  if(!MQTT.connected()){
    sendDatabase = false;
    MQTT_FlageOK = false;
    SendLogMACCooToBroker = false;
    publicAlertReplayFlage = false;
    // DHCP_FlageOK = false;
  }
  if (Ethernet.linkStatus() == LinkOFF ){
    checkENCCable_Exsiting = false;
    DHCP_FlageOK = false;
    sendDatabase = false;
    MQTT_FlageOK = false;
    SendLogMACCooToBroker = false;
    publicAlertReplayFlage = false;
    client.stop();
    digitalWrite(PB1, LOW);     
  }
  if(Ethernet.hardwareStatus() == EthernetNoHardware){
    SendLogMACCooToBroker = false;
    checkENCHardware_Exsiting = false;
    checkENCCable_Exsiting = false;
    DHCP_FlageOK = false;
    sendDatabase = false;
    MQTT_FlageOK = false;
    publicAlertReplayFlage = false;
  }
}

void MQTTLOOP() {
  MQTT.loop();
}
