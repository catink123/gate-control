#include <ArduinoJson.h>

static const char* TYPE = "type";
static const char* PAYLOAD = "payload";
static const char* CHANGE_STATE = "change_state";
static const char* QUERY_STATE = "query_state";
static const char* QUERY_STATE_RESULT = "query_state_result";

const int capacity = 100;
StaticJsonDocument<capacity> doc;
bool state = false;

void send_state() {
  DynamicJsonDocument dyn_doc(100);
  dyn_doc[TYPE] = QUERY_STATE_RESULT;
  dyn_doc[PAYLOAD] = state;

  String msg;
  serializeJson(dyn_doc, msg);

  Serial.write(msg.c_str());
}

void change_state(bool new_state, bool persist_change = true) {
  if (persist_change) {
    state = new_state;
    send_state();
  }

  if (new_state) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  pinMode(13, OUTPUT);
  change_state(false);
}

void loop() {
  if (Serial.available() > 0) {
    String msg = Serial.readString();

    deserializeJson(doc, msg);

    if (doc[TYPE] == CHANGE_STATE) {
      if (doc[PAYLOAD] == true) {
        change_state(true);
      } else if (doc[PAYLOAD] == false) {
        change_state(false);
      }
    } else if (doc[TYPE] == QUERY_STATE) {
      send_state();
    } else {
      Serial.write("{\"type\": \"error\", \"payload\": \"unknown_command\"}");
    }
  }
}
