
#include "ZigBeeHandle.h"


extern bool ZigBee_init;
extern bool Connected;
extern char MAC_Address[17];
extern bool messageSent;
extern char belongCooMAC[17];
extern bool belongCooFlage;



#include <SoftwareSerial.h>
// SoftwareSerial mySerial(A3, A4); // RX, TX
HardwareSerial Serial2(USART2);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(57600);  //115200);//57600);
  pinMode(PC13, OUTPUT);
  pinMode(PA1, OUTPUT);
  pinMode(PA0, OUTPUT);
  digitalWrite(PA0, LOW);
  digitalWrite(PC13, LOW);
  delay(500);
  digitalWrite(PC13, HIGH);
  delay(500);
  digitalWrite(PC13, LOW);
  delay(500);
  digitalWrite(PC13, HIGH);
  delay(500);
  digitalWrite(PC13, LOW);

  digitalWrite(PA1, HIGH);
  delay(250);
  digitalWrite(PA1, LOW);
  delay(2000);
}

void loop() {
  if (!ZigBee_init && !Connected) {
    initZigBee();
  }

  if (ZigBee_init && Connected && !messageSent) {
    send_log_message();
  }

  if (ZigBee_init && Connected && messageSent) {
    HandleMessages();
  }

  if (belongCooMAC[0] != '\0' && !belongCooFlage) {
    CommToZigBee("+++\r\n", false);
    while (!reconnecting_belongCoo())
      ;
  }
}
