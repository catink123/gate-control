#include <ArduinoJson.h>
const int capacity = 100;
StaticJsonDocument<capacity> doc;
bool state = false;

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('%');
    // skip the message end character
    Serial.read();

    deserializeJson(doc, msg);
    if (doc["type"] == "change_state" && doc["payload"] == true) {
      digitalWrite(13, HIGH);
      state = true;
    } else if (doc["type"] == "change_state" && doc["payload"] == false) {
      digitalWrite(13, LOW);
      state = false;
    } else if (doc["type"] == "query_state") {
      DynamicJsonDocument dyn_doc(100);
      dyn_doc["type"] = "query_state_result";
      dyn_doc["payload"] = state;
      String msg;
      serializeJson(dyn_doc, msg);
      msg += '%';

      Serial.write(msg.c_str());
    } else {
      Serial.write("{\"type\": \"error\", \"payload\": \"unknown_command\"}%");
    }
  }
}
