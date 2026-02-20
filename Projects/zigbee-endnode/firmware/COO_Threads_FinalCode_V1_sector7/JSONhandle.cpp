
#include "JSONHandle.h"

bool checkJSONFormat(char *JSONmessage){
  StaticJsonDocument<512> doc; // allocate memory for JSON parsing
  //Serial.println(JSONmessage);
  DeserializationError error = deserializeJson(doc, JSONmessage);

  if (error) {
    Serial.println("\n\n❌ Not JSON\n");
    //Serial.println(error.c_str());
    //Serial.println();
    return false;
  } else {
    //serializeJsonPretty(JSONmessage, Serial);
    //Serial.println();
    //Serial.print(". Message: ");
    //serializeJsonPretty(doc, Serial);
    //Serial.println(doc);
    Serial.println("\n\n✅ Valid JSON\n");
    // optional: print parsed content
    
    //Serial.println();
    return true;
  }
}

static bool isValidMac(const char *mac) {
  if (strlen(mac) != 16) return false;
  for (size_t i = 0; i < 16; i++) {
    if (!isxdigit(mac[i])) return false;
  }
  return true;
}

JsonData parseJson(const char *msg) {
  StaticJsonDocument<512> doc;
  JsonData data = {"", "", "", "", false};

  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("❌ JSON Parse Error: ");
    Serial.println(error.c_str());
    return data;
  }

  // استخراج القيم بأمان
  if (doc.containsKey("mac"))
    strncpy(data.mac, doc["mac"], sizeof(data.mac) - 1);

  if (doc.containsKey("command"))
    strncpy(data.command, doc["command"], sizeof(data.command) - 1);

  if (doc.containsKey("userId"))
    strncpy(data.userId, doc["userId"], sizeof(data.userId) - 1);

  if (doc.containsKey("transactionId"))
    strncpy(data.transactionId, doc["transactionId"], sizeof(data.transactionId) - 1);

  // تحقق MAC
  data.validMac = isValidMac(data.mac);

  return data;
}