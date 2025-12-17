#include <STM32FreeRTOS.h>
#include <ZigBeeHandle.h>
#include <EthernetHandle.h>
#include <iostream>
#include <string>
#include <ArduinoJson.h>
#include "JSONHandle.h"



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
HardwareSerial Serial2(PA3, PA2); // RX, TX

extern char MAC_Address[17];
extern bool getByteMAC;


static void task1(void *pvParameters) {
  for (;;) {
      indicators();
    //vTaskDelay(pdMS_TO_TICKS(100));
    //vTaskDelay(pdMS_TO_TICKS(1));
  }
}
 
static void task2(void *pvParameters) {
  for (;;) {
    if (ZigBee_init == false){
      initZigBee();
      vTaskDelay(100);
    }
    else if(ZigBee_init == true){   
      HandlEveryIncoming();
      handleOpenDoor(); 
      //deletedDevicesProccessing();
      //vTaskDelay(pdMS_TO_TICKS(100));
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    //vTaskDelay(pdMS_TO_TICKS(100));
  }
}


static void task3(void *pvParameters) {
  for (;;) {
    TimersForNodes();
    //scanningTheNotbelongDatabase();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
 

static void task4(void *pvParameters) {
  for (;;) {
    if(ZigBee_init == true && !DHCP_FlageOK && !MQTT_FlageOK){
      //digitalWrite(LED_PIN, HIGH);
      ENC_Connect();
    }
    else if(DHCP_FlageOK && !MQTT_FlageOK){
      MQTT_ConnectProccessing();
      if(belongNodesNumber == 0 )
      {
        SendLogMACCooToBroker = false;
      }
    }
    else if(!SendLogMACCooToBroker && MQTT_FlageOK){
      PublishMACCooRoBroker();
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    else if (!sendDatabase && SendLogMACCooToBroker && MQTT_FlageOK){
      sendDataBASE_AfterConnectionIsDone();
    }
    else if(sendDatabase && SendLogMACCooToBroker && MQTT_FlageOK) {
      //UpdateNetwork();
      SendNewMacEndDevice();
    }
    MQTTLOOP();
    scaningConnection();
    //runningEveryTimeUntilAlertOn();
    //runningEveryTimeUntilFailureOn();
    vTaskDelay(pdMS_TO_TICKS(1));
    //vTaskDelay(1);
  }
}

 
void setup() 
{
  delay(1000);
  Serial.begin(115200);
  //mySerial.begin(57600);
  Serial2.begin(57600);
  delay(2000);
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(PB1, OUTPUT); // LED connect to pin PB1
  pinMode(LED_PIN, OUTPUT);
  deviceCount = 0;
  /*digitalWrite(PB1, HIGH);
  delay(2000);
  digitalWrite(PB1, LOW);
  delay(2000);*/
  xTaskCreate(task1,"Task1",
              configMINIMAL_STACK_SIZE,NULL,tskIDLE_PRIORITY + 2,NULL);
  xTaskCreate(task2, "Task2", 512, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(task3, "Task3", 512, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(task4, "Task4", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
  //xTaskCreate(task5, "Task5", 512, NULL, tskIDLE_PRIORITY + 2, NULL);
  vTaskStartScheduler();
}
 
void loop() 
{
}