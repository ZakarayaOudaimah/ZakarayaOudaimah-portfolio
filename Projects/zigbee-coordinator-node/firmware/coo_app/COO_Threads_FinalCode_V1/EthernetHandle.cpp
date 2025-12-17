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

#define MYIPADDR 192,168,1,28
#define MYIPMASK 255,255,255,0
#define MYDNS 192,168,1,1
#define MYGW 192,168,1,1
#define PublishTopicLOG "m@dernl0ck/databse-inquiry/"
//#define PublishDatabase "m@dernl0ck/coos/log"
//#define SubscribeTopicOPENDoor "ModernLock_Door_Action"

char server[] = "dace514c6516469bae194ca09c8c889a.s1.eu.hivemq.cloud";
#define MQTT_PORT  8883
const int rand_pin = A0;
EthernetClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, rand_pin);

void MQTTcallback(char* topic, byte* payload, unsigned int length);
PubSubClient MQTT(server,MQTT_PORT,MQTTcallback,client);

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
extern bool sendDatabase;
bool publicAlertReplayFlage = false;



extern char MAC_Address[17];
char messageArriverOnMACCooTopic[512];


struct domains {
  char smartLock[10] = "smartLock";
};

domains domainType;



void MQTTcallback(char* topic, byte* payload, unsigned int length) {
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
    if((strstr(topic , "connection-joining") != NULL) && (strstr(topic , MAC_Address) != NULL)){
      messageArrivedOnBleongsNodes = true;
    }
    else{
      messageArrivedOnBleongsNodes = false;
    }

    if((strstr(topic , "connection-deleting") != NULL) && (strstr(topic , MAC_Address) != NULL)){
      messageArrivedOnDeletingOnBleongsNodes = true;
    }
    else{
      messageArrivedOnDeletingOnBleongsNodes = false;
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
    messageArriverOnMACCooTopic[length] = '\0'; // إضافة نهاية string
    //Serial.println(messageArriverOnMACCooTopic);
    Serial.println();
    //messageArrivedOnTopic = true;
  //}
}

void ConvertStringMACtoCharr(const char *macString){
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




void PublishDataBase(char *database){
  char ModernLock[50] = "m@dernl0ck/database/";
  char COOTopics[128] = {0};
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nPublish the MAC to Brocker on Database Topic : ");
  Serial.print(COOTopics);
  Serial.print(", Message : ");
  Serial.print(database);
  Serial.println("\n");
  MQTT.publish( COOTopics , database );
}


void PublishOnNewNodeAdded(char *MacNewNode){
  char ModernLock[50] = "m@dernl0ck/connection-joining-reply/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nPublish the MAC to Brocker on NewNodes Topic : ");
  Serial.print(COOTopics);
  Serial.print(", Message : ");
  Serial.print(MacNewNode);
  Serial.println("\n");
  MQTT.publish( COOTopics , MacNewNode );
}

void PublishUpdateOnNetwork(char *UpdateMessga ){
  char ModernLock[50] = "m@dernl0ck/update/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nPublish the MAC to Brocker on Update Topic : ");
  Serial.print(COOTopics);
  Serial.print(", Message : ");
  Serial.print(UpdateMessga);
  Serial.println("\n");
  MQTT.publish( COOTopics , UpdateMessga );
}


/*void PublishAlertMessages(char *AlertMessga){
  char ModernLock[50] = "m@dernl0ck/devices/alert";
  char COOTopics[128] = {0};
  strcat(COOTopics, ModernLock);
  Serial.print("\nPublish the alert Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.publish( COOTopics , AlertMessga );
}

void PublishCommandReply(char *CommandReplyMessga){
  char ModernLock[50] = "m@dernl0ck/devices/commandReply";
  char COOTopics[128] = {0};
  strcat(COOTopics, ModernLock);
  Serial.print("\nPublish the commandReply Topic : ");
  Serial.print(COOTopics);
  //Serial.print(", Message : ");
  //Serial.print(CommandReplyMessga);
  Serial.println("\n");
  MQTT.publish( COOTopics , CommandReplyMessga );
}*/


void publishDevices(char *domain , char *mess){
  char ModernLock[50] = "m@dernl0ck/devices/";
  char COOTopics[256] = {0};
  
  if (strncmp(domain, "smartLock", 9) == 0){
    strcat(COOTopics, ModernLock);
    strcat(COOTopics, domainType.smartLock);
    Serial.print("\nPublish m@dernl0ck/devices/{Domain} Topic on: ");
    Serial.println(COOTopics);
    MQTT.publish( COOTopics , mess );
  }
  else if(strncmp(domain, "thermoState", 11) == 0){}
  else{}
}

void SubscirbeOnPassWordAdded(){
  char ModernLock[50] = "m@dernl0ck/password/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\Subsecribe to the MAC of Coordinator to Brocker on PassWord Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe( COOTopics );

}

void SubsecribeOnOpenDoor(){
  char ModernLock[50] = "m@dernl0ck/opendoor/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on OpenDoor Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe( COOTopics );

}

void SubsecribeOnBelongNodesTopic(){
  char ModernLock[50] = "m@dernl0ck/connection-joining/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on BelongNodes Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe( COOTopics );
}

void SubsecribeOnDeletingBelongNodesTopic(){
  char ModernLock[50] = "m@dernl0ck/connection-deleting/";
  char COOTopics[128] = {0};
  // add new node 
  strcat(COOTopics, ModernLock);
  strcat(COOTopics, MAC_Address);
  Serial.print("\nSubsecribe to the MAC of Coordinator to Brocker on deleting from BelongNodes Topic : ");
  Serial.print(COOTopics);
  Serial.println("\n");
  MQTT.subscribe( COOTopics );

}

void MQTT_ConnectProccessing(){
  /*if(MQTT.connected()){
    MQTT.disconnect();
    client.stop();
    vTaskDelay(1000);
  }*/
  if (!MQTT.connected()) {
    Serial.println("Attempting MQTT connection...");
    vTaskDelay(1000);
    if (MQTT.connect("ZAKARAYA","zakaraya","oudaimah")){//, "modernlock", "M0dernl@ck12345"))
      vTaskDelay(1000);
      Serial.println("connected");
      //MQTT.subscribe(MAC_Address);
      SubscirbeOnPassWordAdded();
      SubsecribeOnOpenDoor();
      SubsecribeOnBelongNodesTopic();
      SubsecribeOnDeletingBelongNodesTopic();
      MQTT_FlageOK = true;
      publicAlertReplayFlage = true;
    }
    else {
      Serial.print("failed, rc=");
      Serial.println(MQTT.state());
      vTaskDelay(3000);
      MQTT_FlageOK = false;
    }
    vTaskDelay(10);
  }
}


bool checkENCHardwareExsiting(){
  Ethernet.init(PA4);   // CS pin تبع ENC28J60

  // جرب تقرأ حالة الهاردوير
  EthernetHardwareStatus hw = Ethernet.hardwareStatus();
  //Ethernet.init(PA4);
  //Ethernet.end();
  Ethernet.init(PA4);
  if(hw  == EthernetNoHardware) {
    Serial.print("Ethernet shield was not found.  Sorry, can't run without hardware. :( ");
    //Serial.print(hw);
    Serial.print("\n");
    digitalWrite(PC13, HIGH);
    vTaskDelay(500);
    digitalWrite(PC13, LOW);
    vTaskDelay(500);
    return false;
  }
  else{
    Serial.print("Ethernet shield was found. ENC28J60. :( ");
    //Serial.print(hw);
    Serial.print("\n");
    return true;
  }
}

bool checkENCCableExsiting(){
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
  }
  else if (Ethernet.linkStatus() == LinkON){
    Serial.println("Ethernet Cable was found. ENC28J60");
    return true;
  }
}
void ENC_Connect(){
  //Ethernet.end();
  //base_client.end();
  if(!checkENCHardware_Exsiting){
    while(!checkENCHardware_Exsiting){
      checkENCHardware_Exsiting = checkENCHardwareExsiting();
    }
  }

  if(checkENCHardware_Exsiting && !checkENCCable_Exsiting){
    while(!checkENCCable_Exsiting){
      checkENCCable_Exsiting = checkENCCableExsiting();
    }
  }

  if(checkENCHardware_Exsiting && checkENCCable_Exsiting && !DHCP_FlageOK){
    Serial.println("\n\nProcessing for DHCP Connections ....... \n");
    if (Ethernet.begin(ByteMAC)) { // Dynamic IP setup
      Serial.println("DHCP OK!");
      digitalWrite(PB1, HIGH);
      DHCP_FlageOK = true;
    }
    else{
      IPAddress ip(MYIPADDR);
      IPAddress dns(MYDNS);
      IPAddress gw(MYGW);
      IPAddress sn(MYIPMASK);
      Ethernet.begin(ByteMAC, ip, dns, gw, sn);
      Serial.println("STATIC OK!");
    }
    Serial.print("Local IP : ");
    Serial.println(Ethernet.localIP());
    Serial.print("Subnet Mask : ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("Gateway IP : ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS Server : ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.println("Ethernet Successfully Initialized");
    vTaskDelay(1000);
  }

  /*if(checkENCHardware_Exsiting && checkENCCable_Exsiting && DHCP_FlageOK && !MQTT_FlageOK){
    //Serial.println("\n\nProcessing for Broker Connections ....... \n");
    while(!MQTT_FlageOK){
      MQTT_FlageOK = MQTT_ConnectProccessing();
      vTaskDelay(3000);
    }
  }*/
}


void PublishMACCooRoBroker(){
  StaticJsonDocument<256> doc;
  if(SendLogMACCooToBroker == false && MQTT_FlageOK){

    char ModernLock[128] = "m@dernl0ck/databse-inquiry/";
    char COOTopics[128] = {0};
    strcat(COOTopics, ModernLock);
    strcat(COOTopics, MAC_Address);
    Serial.print("\nPublish the MAC to Brocker on databse-inquiry Topic : ");
    Serial.print(COOTopics);
    Serial.print(", Message : ");
    Serial.print(NULL);
    Serial.println("\n");
    MQTT.publish( COOTopics , NULL );
    /*// Add the key-value pairs
    doc["mac"] = MAC_Address;

    // Create a character array to hold the serialized JSON string
    char jsonBuffer[256];

    // Serialize the JSON document into the buffer
    serializeJson(doc, jsonBuffer);
    Serial.print("\nPublish the MAC of Coordinator to Brocker on LOG Topic : ");
    Serial.print(jsonBuffer);
    Serial.println("\n");
    MQTT.publish( PublishTopicLOG , jsonBuffer );*/
    SendLogMACCooToBroker = true;
  }
}



void scaningConnection(){
  Ethernet.init(PA4);
  if(!MQTT.connected()){
    sendDatabase = false;
    MQTT_FlageOK = false;
    publicAlertReplayFlage = false;
    //DHCP_FlageOK = false;
  }
  //checkENCCable_Exsiting = checkENCCableExsiting();
  if (Ethernet.linkStatus() == LinkOFF ){
    checkENCCable_Exsiting = false;
    DHCP_FlageOK = false;
    sendDatabase = false;
    MQTT_FlageOK = false;
    publicAlertReplayFlage = false;
    client.stop();
    /*if(MQTT.connected()){
      MQTT.disconnect();
    }*/
    digitalWrite(PB1, LOW);     
  }
  if(Ethernet.hardwareStatus() == EthernetNoHardware){
    checkENCHardware_Exsiting = false;
    checkENCCable_Exsiting = false;
    DHCP_FlageOK = false;
    sendDatabase = false;
    MQTT_FlageOK = false;
    publicAlertReplayFlage = false;
  }
}

void MQTTLOOP(){
  MQTT.loop();
}
