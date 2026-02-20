
#include "ZigBeeHandle.h"
#include <LowPower.h>
extern bool ZigBee_init;
extern bool Connected;
extern char MAC_Address[17];
extern bool messageSent;
extern char belongCooMAC[17];
extern bool belongCooFlage;




#include <SoftwareSerial.h>
SoftwareSerial mySerial(A3, A4);  // RX, TX
// HardwareSerial mySerial(USART2);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(57600);  //115200);//57600);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  //test
  pinMode(12, OUTPUT);
  //endtest

  pinMode(2, INPUT_PULLUP);
  // pinMode(2, INPUT);
  pinMode(A1, INPUT_PULLUP);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);

  delay(2000);
}

void loop() {
  // Serial.println("\nSleep");
  // delay(10);
  // SleepWithRecievetest();

  if (!ZigBee_init && !Connected) {
    initZigBee();
  }

  if (ZigBee_init && Connected && !messageSent) {
    send_log_message();
  }

  if (ZigBee_init && Connected && messageSent) {
    HandleMessages();
    if(ZigBee_init){
      Checking();
    }
  }

  if (belongCooMAC[0] != '\0' && !belongCooFlage) {
    CommToZigBee("+++\r\n", false);
    while (!reconnecting_belongCoo())
      ;
  }
}


void wakeUptest() {
  // Serial.println("\nWAKE");
  
}

void SleepWithRecievetest() {
  // Serial.println("\nSleep");
  digitalWrite(12, LOW);
  attachInterrupt(0, wakeUptest, LOW);
  

  delay(10);

  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  detachInterrupt(0);
}
