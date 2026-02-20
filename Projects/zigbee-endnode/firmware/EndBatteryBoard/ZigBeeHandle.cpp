// #include "Hardware//Serial.h"
#include "Arduino.h"
#include <LowPower.h>
#include "ZigBeeHandle.h"
#include "SoftwareSerial.h"


#include <Keypad.h>
// #include <LowPower.h>

const byte ROWS = 4;  //four rows
const byte COLS = 3;  //three columns
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

byte rowPins[ROWS] = { 11, 10, 13, 12 };  //connect to the row pinouts of the keypad
byte colPins[COLS] = { 7, 3, 2 };     //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


#define new_node "N,MAC:"
#define analog_mess "B,MAC:"
#define batt_noti_mess "I,MAC:"
#define command_mess "C,MAC:"
#define sleep_atr "SL:"
#define node_kind "D"  //Door
#define max_pass_length 8
#define analog_batt A5

extern SoftwareSerial mySerial;

// Flags - already minimal
bool ZigBee_init;
bool joined = false;
bool getMAC = false;
bool Connected = false;
bool messageSent = false;
bool hardwareZigBee = false;
bool reScanNetworks = false;

// Reduced buffer sizes where possible
char passWordOnKeypad[8] = { 0 };  // Already minimal
char MAC_Address[17] = { 0 };      // 16 chars + null terminator
char belongCooMAC[17] = { 0 };     // Same as MAC_Address
uint8_t index_of_belong_coo = 0;
bool belongCooFlage = false;

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

bool isConnectedToCoordinator(const char *mess) {
  char *p_pan = strstr(mess, "PANID:");
  char *p_expan = strstr(mess, "EXPANID:");

  if (!p_pan || !p_expan) {
    //Serial.println("!P/E");
    return false;
  }

  p_pan += strlen("PANID: ");
  if (p_pan[0] == '0' && (p_pan[1] == 'x' || p_pan[1] == 'X'))
    p_pan += 2;

  char panid[5] = { 0 };
  strncpy(panid, p_pan, 4);

  p_expan += strlen("EXPANID: ");
  if (p_expan[0] == '0' && (p_expan[1] == 'x' || p_expan[1] == 'X'))
    p_expan += 2;

  char expanid[17] = { 0 };
  strncpy(expanid, p_expan, 16);

  // Validate
  if (!isHexString(panid, 4) || isAllZeros(panid, 4)) {
    //Serial.print("!P:");
    //Serial.println(panid);
    return false;
  }

  if (!isHexString(expanid, 16) || isAllZeros(expanid, 16)) {
    //Serial.print("!E:");
    //Serial.println(expanid);
    return false;
  }
  Serial.print("P:");
  Serial.println(panid);
  Serial.print("E:");
  Serial.println(expanid);

  return true;
}

void MAC_ZigBee(char *Mess) {
  char *p = strstr(Mess, "Mac Addr");
  if (!p) {
    //Serial.println("MAC not found");
    getMAC = false;
    return;
  }

  while (*p && *p != ':') p++;
  if (!*p) {
    getMAC = false;
    return;
  }

  p += 4;  // Skip ": 0x"
  for (int i = 0; i < 16 && *p; i++) {
    MAC_Address[i] = *p++;
  }
  MAC_Address[16] = '\0';

  // Validate
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

  Serial.print("MAC:");
  Serial.println(MAC_Address);
  getMAC = true;
}

void getIndexFromMAC(char *buffer, const char *desiredMAC) {
  char *line = strtok(buffer, "\n");  // split by lines

  while (line != NULL) {

    // Check if this line contains the desired MAC
    if (strstr(line, desiredMAC) != NULL) {

      // Find "Index: "
      char *indexPtr = strstr(line, "Index:");
      if (indexPtr != NULL) {
        indexPtr += 6;                        // move after "Index:"
        while (*indexPtr == ' ') indexPtr++;  // skip spaces

        index_of_belong_coo = (uint8_t)atoi(indexPtr);  // convert to number
      }
      belongCooFlage = true;
      return;
    } else {
      belongCooFlage = false;
    }

    line = strtok(NULL, "\n");  // next line
  }
}

//Is there any network
void isThereNetworksDiscovered(char mess[]) {
  ////Serial.println("\nmess is :\n");
  Serial.println(mess);
  //char *ptr = mess;
  if (strstr(mess, "Index") != NULL) {
    Serial.println("\nDiscovered\n");
    reScanNetworks = true;
  } else {
    reScanNetworks = false;
  }

  if (belongCooMAC[0] == '\0') {
    index_of_belong_coo = 0;
  } else {
    getIndexFromMAC(mess, belongCooMAC);
  }
}




// Send Command
void CommToZigBee(char *Mess, bool isConnected) {
  char response[512] = { 0 };
  int counter_response = 0;
  bool messageRecieve = false;
  unsigned long lastTimeforResponse = millis();
  if (strncmp(Mess, "ATRS", 4) == 0 && isConnected == false) {
    // Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    lastTimeforResponse = millis();
    while (!messageRecieve && (millis() - lastTimeforResponse <= 5000)) {
      while (mySerial.available()) {
        char c = mySerial.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response++] = c;
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
      } else continue;
    }
  } else if (strncmp(Mess, "ATLN", 4) == 0 && isConnected == false) {
    //Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    while (!messageRecieve) {
      while (mySerial.available()) {
        char c = mySerial.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response++] = c;
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Serial.println("\nGoing to discover networks\n");
        isThereNetworksDiscovered(response);
      } else continue;
    }
  } else if (strncmp(Mess, "ATJN", 4) == 0 && isConnected == false) {
    Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    while (!messageRecieve) {
      while (mySerial.available()) {
        char c = mySerial.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        joined = true;
        response[0] == '\0';
        //Serial.println();
        break;
      } else if (strstr(response, "Error") != NULL) {
        //Serial.println("\nSending ATRS Command again .. \n");
        joined = false;
        break;
      } else continue;
    }
  } else if (strncmp(Mess, "ATIF", 4) == 0 && isConnected == true) {
    // Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    while (!messageRecieve) {
      while (mySerial.available()) {
        char c = mySerial.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Connected = isConnectedToCoordinator(response);

        //Serial.println();
        //Serial.println();
      } else continue;
    }
  } else if (strncmp(Mess, "ATIF", 4) == 0 && isConnected == false) {
    //Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    while (!messageRecieve) {
      while (mySerial.available()) {
        char c = mySerial.read();
        ////Serial.write(c);
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        MAC_ZigBee(response);
        Serial.println();
        break;
      } else continue;
    }
  } else {
RepateSending:
    lastTimeforResponse = millis();
    Serial.print(Mess);
    delay(10);
    mySerial.print(Mess);
    while (!messageRecieve && (millis() - lastTimeforResponse <= 1000)) {
      while (mySerial.available()) {
        char c = mySerial.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
        if (strncmp(Mess, "+++", 3) == 0) {
          if (strstr(response, "Error, invalid command") != NULL || strstr(response, "Enter AT Mode.") != NULL) {
            ////Serial.print(response);
            messageRecieve = true;
            hardwareZigBee = true;
            //Serial.println();
            break;
          } else continue;
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        //Serial.println();
        break;
      } else continue;
    }
    if (response[0] == '\0')  // && ( millis() - lastTimeforResponse >= 1000) ){
    {
      Serial.println("There is no hardware please check please .... \n");
      hardwareZigBee = false;
      ZigBee_init = false;
      messageSent = false;
      lastTimeforResponse = millis();
      goto RepateSending;
    }
  }
}

bool reconnecting_belongCoo() {
  Connected = false;
  CommToZigBee("ATRS\r\n", false);
  CommToZigBee("ATLN\r\n", false);
  if (belongCooFlage && reScanNetworks) {
    char subcommand[16];  // make buffer larger for safety
    sprintf(subcommand, "ATJN%u\r\n", index_of_belong_coo);
    CommToZigBee(subcommand, false);
    int attempts = 0;
    while (!Connected && attempts++ < 3) {
      CommToZigBee("ATIF\r\n", true);
      delay(1000);
      if (!Connected && attempts >= 3) {
        reconnecting_belongCoo();
        attempts = 0;
      }
    }
  } else {
    delay(3000);
    Serial.println("belongCoo Not Found");
    return false;
  }
  if (Connected) {
    CommToZigBee("ATDT\r\n", false);
    return true;
  }
}

void reScan(uint8_t indexCoo) {
  CommToZigBee("ATRS\r\n", false);
  CommToZigBee("ATLN\r\n", false);
  if (reScanNetworks) {
    char subcommand[16];  // make buffer larger for safety
    sprintf(subcommand, "ATJN%u\r\n", index_of_belong_coo);
    CommToZigBee(subcommand, false);
  }
  if (!joined) {
    reScan(index_of_belong_coo);
  }
}

void initZigBee() {
  digitalWrite(5, HIGH);
  delay(500);
  digitalWrite(5, LOW);
  while (!hardwareZigBee) {
    CommToZigBee("+++\r\n", false);
    delay(1000);
  }

  while (!getMAC) {
    CommToZigBee("ATIF\r\n", false);
    delay(1000);
  }

  CommToZigBee("ATSP8000\r\n", false);
  CommToZigBee("ATSM0\r\n", false);
  CommToZigBee("ATDA0000\r\n", false);
  CommToZigBee("ATTM1\r\n", false);
  CommToZigBee("ATAJ0\r\n", false);

  reScan(index_of_belong_coo);
  delay(2000);

  ZigBee_init = true;
  int attempts = 0;
  while (!Connected && attempts++ < 3) {
    CommToZigBee("ATIF\r\n", true);
    delay(1000);
    if (!Connected && attempts >= 3) {
      reScan(index_of_belong_coo);
      attempts = 0;
    }
  }

  CommToZigBee("ATDT\r\n", false);
  Serial.println("\n\nREADY");
}


void Checking() {
  Serial.print("\nChecking\n");
  CommToZigBee("+++\r\n", false);
  CommToZigBee("ATIF\r\n", true);
  if (!Connected) {
    hardwareZigBee = false;
    ZigBee_init = false;
    Connected = false;
    messageSent = false;
    return;
  }
  CommToZigBee("ATDT\r\n", false);
}

void OpenDoor() {
  //Serial.println("OPEN");
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(9, LOW);
  digitalWrite(8, HIGH);
  delay(200);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  delay(2500);

  //Serial.println("CLOSE");
  digitalWrite(4, HIGH);
  delay(100);
  digitalWrite(9, HIGH);
  digitalWrite(8, LOW);
  delay(200);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);

  // Cleanup
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(4, LOW);
}
char buffer[64];
char battStr[8];

void log_message() {
  snprintf(buffer, sizeof(buffer), "%s%s,%s1%s", new_node, MAC_Address, sleep_atr, node_kind);
  //Serial.println(buffer);
  mySerial.println(buffer);
}

void send_batt_noti(float batt) {
  dtostrf(batt, 4, 1, battStr);
  snprintf(buffer, sizeof(buffer), "%s%s,%s", batt_noti_mess, MAC_Address, battStr);
  //Serial.println(buffer);
  mySerial.println(buffer);
}



void extractSecondMAC(char *msg) {

  // Find first comma (after D)
  char *firstComma = strchr(msg, ',');
  if (firstComma == NULL) return;

  // Find second comma (before second MAC)
  char *secondComma = strchr(firstComma + 1, ',');
  if (secondComma == NULL) return;

  secondComma++;  // move to start of second MAC

  // Copy exactly 16 characters (MAC length)
  strncpy(belongCooMAC, secondComma, 16);
  belongCooMAC[16] = '\0';  // null terminate
}

void send_analog_read() {
  float batt = (analogRead(analog_batt) * 3.30) / 1023.00;
  batt = (batt - 2.00) / (3.20 - 2.00);
  batt = batt * 100.0 + 2;
  if (batt >= 99.0) batt = 100.0;

  dtostrf(batt, 0, 1, battStr);  // width = 0 → بدون فراغات
  snprintf(buffer, sizeof(buffer), "%s%s,%s", analog_mess, MAC_Address, battStr);

  Serial.println(buffer);  // للتأكد
  mySerial.println(buffer);
}

bool parse_password(char *mess) {
  memset(passWordOnKeypad, 0, sizeof(passWordOnKeypad));
  char *p = strstr(mess, ",02");
  if (!p) return false;

  p += 3;
  int i = 0;
  while (*p && *p != '\n' && i < max_pass_length) {
    passWordOnKeypad[i++] = *p++;
    if (i == 0 || i > max_pass_length) {
      passWordOnKeypad[0] = '\0';
      i = 0;
      //Serial.println("\nYour pass is too big");
      return false;
    }
  }

  Serial.print("PASS: ");
  Serial.println(passWordOnKeypad);
  return true;
}

void handle_lines(char *mess) {
  char *newlinePtr;

  while ((newlinePtr = strchr(mess, '\n')) != NULL) {

    // -------- استخراج السطر --------
    int lineLength = newlinePtr - mess;

    // تجاهل السطر الفارغ
    if (lineLength == 0) {
      memmove(mess, newlinePtr + 1, strlen(newlinePtr + 1) + 1);
      continue;
    }

    char line[256];
    if (lineLength >= sizeof(line))
      lineLength = sizeof(line) - 1;

    memcpy(line, mess, lineLength);
    line[lineLength] = '\0';

    // إزالة السطر من البفر
    memmove(mess, newlinePtr + 1, strlen(newlinePtr + 1) + 1);

    // -------- إزالة \r والمسافات --------
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' '))
      line[--len] = '\0';

    // تجاهل الأسطر الفارغة بالكامل
    if (len == 0) continue;

    Serial.print("\nReceived Line: ");
    Serial.println(line);

    // -------- البحث عن أي MAC في السطر --------
    char *macPtr = line;
    bool foundMyMAC = false;
    while ((macPtr = strstr(macPtr, "MAC:")) != NULL) {
      macPtr += 4;  // تجاوز كلمة MAC:

      // ابحث عن الفاصلة بعد MAC
      char *macEnd = strchr(macPtr, ',');
      if (!macEnd) break;

      if ((macEnd - macPtr) == 16) {
        char mac[17] = { 0 };
        memcpy(mac, macPtr, 16);
        mac[16] = '\0';

        // تحقق من كون MAC hex
        bool valid = true;
        for (uint8_t i = 0; i < 16; i++) {
          if (!isxdigit(mac[i])) {
            valid = false;
            break;
          }
        }
        if (!valid) {
          macPtr = macEnd;  // انتقل للبحث عن MAC آخر
          continue;
        }

        // تحقق إذا هذا MAC يخصني
        if (memcmp(mac, MAC_Address, 16) == 0) {
          foundMyMAC = true;
          break;  // وجدنا MAC الخاص بي
        }
      }

      macPtr = macEnd;  // تابع البحث بعد الفاصلة
    }

    if (!foundMyMAC) continue;  // هذا السطر ليس لي

    // -------- استخراج الأمر بعد آخر فاصلة --------
    char *cmdStart = strrchr(line, ',');
    if (!cmdStart) continue;
    cmdStart++;  // تجاوز الفاصلة

    // -------- تنفيذ الأوامر --------
    if (strncmp(cmdStart, "00", 2) == 0) {
      // Serial.println("✅ Confirmation 00 received");
      messageSent = true;
    } else if (strncmp(cmdStart, "01", 2) == 0) {
      // Serial.println("✅ Command 01 received");
      OpenDoor();
    } else if (strncmp(cmdStart, "02", 2) == 0) {
      // Serial.println("✅ Command 02 received");
      parse_password(line);
    } else if (strncmp(cmdStart, "03", 2) == 0) {
      // Serial.println("✅ Command 03 received");
      send_analog_read();
    } else if (strncmp(cmdStart, "FF", 2) == 0) {
      // Serial.println("✅ Command FF received");
      
      // CommToZigBee("+++\r\n", false);
      hardwareZigBee = false;
      ZigBee_init = false;
      Connected = false;
      messageSent = false;
      delay(5000);
      // initZigBee();
    } else if (line[0] == 'D') {
      // Serial.println("✅ D Message received");
      extractSecondMAC(line);
    }
  }
}



//Send Log message to Coo
void send_log_message() {
  unsigned long start = millis();
  while (!messageSent && (millis() - start < 30000)) {  // 30s timeout
    Checking();
    log_message();
    char response[256] = { 0 };
    response[0] = '\0';
    int idx = 0;
    unsigned long waitStart = millis();

    while (millis() - waitStart < 6000 && idx < sizeof(response) - 1) {
      while (mySerial.available()) {
        char c = mySerial.read();
        Serial.print(c);
        int len = strlen(response);
        if (len < sizeof(response) - 1) {
          response[len] = c;
          response[len + 1] = '\0';
        }
      }
      handle_lines(response);
      if (messageSent) {
        digitalWrite(5, HIGH);
        delay(500);
        digitalWrite(5, LOW);
        delay(500);
        digitalWrite(5, HIGH);
        delay(500);
        digitalWrite(5, LOW);
        break;
      }
    }
    Checking();
  }
}

void Keypad_get_password() {
  Serial.println("\nKeypad_get_password");
  unsigned long millisfor6secs = millis();
  while (millis() - millisfor6secs < 8000) {
    char key = keypad.getKey();

    if (key) {
      Serial.println(key);
    }
  }
}

void HandleMessages() {
  char response[256] = { 0 };
  response[0] = '\0';
  int counter_response = 0;
  //Serial.println("\nSleep Now");
  SleepWithRecieve();
  // Send status
  snprintf(response, sizeof(response), "S,MAC:%s", MAC_Address);
  Serial.println();
  Serial.println(response);
  delay(10);
  mySerial.println(response);
  delay(10);
  unsigned long millisfor6secs = millis();
  while (millis() - millisfor6secs < 1000) {
    while (mySerial.available()) {
      char c = mySerial.read();
      Serial.print(c);
      int len = strlen(response);
      if (len < sizeof(response) - 1) {
        response[len] = c;
        response[len + 1] = '\0';
      }
    }
    handle_lines(response);
    char key = keypad.getKey();

    if (key) {
      Keypad_get_password();
    }

    // float batt = (analogRead(analog_batt) * 3.3 * 100.0) / 1023.0;
    // batt = (batt - 200.0) / (320.0 - 200);
    // batt = batt * 100.0;
    // if (batt < 20.0) {
    //   send_batt_noti(batt);
    // }
  }
}



void wakeUp() {
  // Serial.println("\nWAKE");
  // detachInterrupt(0);
  // Keypad_get_password();
}

void SleepWithRecieve() {
  CommToZigBee("+++\r\n", false);
  CommToZigBee("ATSL\r\n", false);
  digitalWrite(12, LOW);
  attachInterrupt(0, wakeUp, LOW);
  
  delay(10);

  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  delay(10);
  detachInterrupt(0);

  if (digitalRead(2) == LOW) {
    Keypad_get_password();
  }

  CommToZigBee("ATDT\r\n", false);
}