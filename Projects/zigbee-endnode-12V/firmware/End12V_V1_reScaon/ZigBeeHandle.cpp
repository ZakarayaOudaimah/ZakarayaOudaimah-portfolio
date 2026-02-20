#include "HardwareSerial.h"
#include "Arduino.h"
// #include <LowPower.h>
#include "ZigBeeHandle.h"
#include "SoftwareSerial.h"


#include <Keypad.h>
// #include <LowPower.h>

const int ROW_NUM = 4;     //four rows
const int COLUMN_NUM = 3;  //three columns



char keys[ROW_NUM][COLUMN_NUM] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

byte pin_rows[ROW_NUM] = { PB1, PB2, PB3, PB4 };  //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = { PB5, PB6, PB7 };  //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);


#define new_node "N,MAC:"
#define command_mess "C,MAC:"
#define ACK "A,MAC:"
#define sleep_atr "SL:"
#define node_kind "D"  //Door
#define max_pass_length 8

bool ZigBee_init;
char passWordOnKeypad[8] = { 0 };
char MAC_Address[17];
char belongCooMAC[17];
uint8_t index_of_belong_coo = 0;
bool belongCooFlage = false;


bool joined = false;
bool getMAC = false;
bool Connected = false;
bool messageSent = false;
bool hardwareZigBee = false;
bool reScanNetworks = false;


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
    Serial.println("MAC address found ... ");
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
    Serial.print("MAC_ZigBee is : ");
    Serial.println(MAC_Address);
    Serial.println();
    getMAC = true;
  } else {
    Serial.println("MAC address Not found ... ");
    getMAC = false;
    return;
  }
}

// Send Command
void CommToZigBee(char *Mess, bool isConnected) {
  char response[512] = { 0 };
  int counter_response = 0;
  bool messageRecieve = false;
  unsigned long lastTimeforResponse = millis();
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
          //response[counter_response] = '\0';
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Serial.println();
        Serial.println();
      } else continue;
    }
  } else if (strncmp(Mess, "ATRS", 4) == 0 && isConnected == false) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    lastTimeforResponse = millis();
    while (!messageRecieve && (millis() - lastTimeforResponse <= 5000)) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response++] = c;
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
      } else continue;
    }
  } else if (strncmp(Mess, "ATLN", 4) == 0 && isConnected == false) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
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
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        joined = true;
        response[0] == '\0';
        Serial.println();
        break;
      } else if (strstr(response, "Error") != NULL) {
        Serial.println("\nSending ATRS Command again .. \n");
        joined = false;
        break;
      } else continue;
    }
  } else if (strncmp(Mess, "ATIF", 4) == 0 && isConnected == true) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Connected = isConnectedToCoordinator(response);

        Serial.println();
        Serial.println();
      } else continue;
    }
  } else if (strncmp(Mess, "ATIF", 4) == 0 && isConnected == false) {
    Serial.print(Mess);
    delay(10);
    Serial2.print(Mess);
    while (!messageRecieve) {
      while (Serial2.available()) {
        char c = Serial2.read();
        //Serial.write(c);
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
    Serial2.print(Mess);
    while (!messageRecieve && (millis() - lastTimeforResponse <= 1000)) {
      while (Serial2.available()) {
        char c = Serial2.read();
        if (counter_response < sizeof(response) - 1) {
          response[counter_response] = c;
          Serial.write(response[counter_response++]);
        }
        if (strncmp(Mess, "+++", 3) == 0) {
          if (strstr(response, "Error, invalid command") != NULL || strstr(response, "Enter AT Mode.") != NULL) {
            //Serial.print(response);
            messageRecieve = true;
            hardwareZigBee = true;
            Serial.println();
            break;
          } else continue;
        }
      }
      if (strstr(response, "OK") != NULL) {
        messageRecieve = true;
        Serial.println();
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

// init ZigBee
void initZigBee() {

Return:
  while (!hardwareZigBee) {
    CommToZigBee("+++\r\n", false);
    delay(1000);
  }
  //CommToZigBee("ATRB\r\n", false);
  while (getMAC == false) {
    CommToZigBee("ATIF\r\n", false);
    delay(1000);
  }

  CommToZigBee("ATSP6000\r\n", false);
  CommToZigBee("ATSM0\r\n", false);
  CommToZigBee("ATDA0000\r\n", false);
  CommToZigBee("ATTM1\r\n", false);
  CommToZigBee("ATAJ0\r\n", false);

  reScan(index_of_belong_coo);
  delay(2000);

  Serial.print("ZigBee_init ");
  Serial.println("true... \n");
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
  Serial.print("DEVICE IS READY...\n\n");
}

void Checking() {
  Serial.print("\nChecking\n");
  CommToZigBee("+++\r\n", false);
  CommToZigBee("ATIF\r\n", true);
  if (!Connected) {
    ZigBee_init = false;
    Connected = false;
    messageSent = false;
    return;
  }
  CommToZigBee("ATDT\r\n", false);
}

void OpenDoor() {
  Serial.println("OPEN");
  digitalWrite(PA0, HIGH);
  delay(3000);
  Serial.println("CLOSE");
  digitalWrite(PA0, LOW);
  delay(100);
}

char buffer[64];


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

void log_message() {
  snprintf(buffer, sizeof(buffer), "%s%s,%s0%s\n", new_node, MAC_Address, sleep_atr, node_kind);
  Serial.print(buffer);
  Serial2.print(buffer);
}

void ACK_message() {
  snprintf(buffer, sizeof(buffer), "%s%s", ACK, MAC_Address);
  Serial.println(buffer);
  Serial2.println(buffer);
}

bool parse_password(char *mess) {
  char *p = strstr(mess, ",02");
  if (!p) return false;

  p += 3;
  int i = 0;
  while (*p && *p != '\n' && i < max_pass_length) {
    passWordOnKeypad[i++] = *p++;
    if (i == 0 || i >= max_pass_length) {
      passWordOnKeypad[0] = '\0';
      i = 0;
      Serial.println("\nYour pass is too big");
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
      Serial.println("✅ Confirmation 00 received");
      messageSent = true;
    } else if (strncmp(cmdStart, "01", 2) == 0) {
      Serial.println("✅ Command 01 received");
      digitalWrite(PA1, HIGH);
      delay(250);
      digitalWrite(PA1, LOW);
      OpenDoor();
      ACK_message();
    } else if (strncmp(cmdStart, "02", 2) == 0) {
      Serial.println("✅ Command 02 received");
      parse_password(line);
      ACK_message();
    } else if (strncmp(cmdStart, "FF", 2) == 0) {
      Serial.println("✅ Command FF received");
      hardwareZigBee = false;
      ZigBee_init = false;
      Connected = false;
      messageSent = false;
      delay(5000);
    } else if (line[0] == 'D') {
      Serial.println("✅ D Message received");
      extractSecondMAC(line);
    }
  }
}


//Send Log message to Coo
void send_log_message() {
  unsigned long start = millis();
  while (!messageSent && (millis() - start < 30000)) {  // 30s timeout
    log_message();
    char response[128] = { 0 };
    response[0] = '\0';
    int idx = 0;
    unsigned long waitStart = millis();

    while (millis() - waitStart < 6000 && idx < sizeof(response) - 1) {
      while (Serial2.available()) {
        char c = Serial2.read();
        Serial.print(c);
        int len = strlen(response);
        if (len < sizeof(response) - 1) {
          response[len] = c;
          response[len + 1] = '\0';
        }
      }
      handle_lines(response);
      if (messageSent) {
        digitalWrite(PA1, HIGH);
        delay(250);
        digitalWrite(PA1, LOW);
        break;
      }
    }
  }
}
  void Keypad_get_password() {
    Serial.println("Keypad_get_password");
    unsigned long millisfor6secs = millis();
    while (millis() - millisfor6secs < 6000) {
      char key = keypad.getKey();

      if (key) {
        Serial.println(key);
      }
    }
  }


  void HandleMessages() {
    char responseBuffer[512] = { 0 };  // Reduced from 512
    responseBuffer[0] = '\0';
    int counter_response = 0;
    unsigned long millisfor6secs = millis();
    // Send status
    snprintf(responseBuffer, sizeof(responseBuffer), "S,MAC:%s", MAC_Address);
    Serial.println(responseBuffer);
    Serial2.println(responseBuffer);
    delay(50);


    while (millis() - millisfor6secs < 8000) {
      while (Serial2.available() > 0) {
        char c = Serial2.read();
        Serial.write(c);
        int len = strlen(responseBuffer);
        if (len < sizeof(responseBuffer) - 1) {
          responseBuffer[len] = c;
          responseBuffer[len + 1] = '\0';
        }
      }
      handle_lines(responseBuffer);
      char key = keypad.getKey();

      if (key) {
        Keypad_get_password();
      }
    }
  }
