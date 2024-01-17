#include <ArduinoJson.h>

static const char* TYPE = "type";
static const char* PAYLOAD = "payload";
static const char* CHANGE_STATE = "change_state";
static const char* QUERY_STATE = "query_state";
static const char* QUERY_STATE_RESULT = "query_state_result";

static const char* ERROR_MESSAGE = "{\"type\": \"error\", \"payload\": \"unknown_command\"}";

static const char* STATE_RAISED = "raised";
static const char* STATE_RAISING = "raising";
static const char* STATE_LOWERED = "lowered";
static const char* STATE_LOWERING = "lowering";

static const unsigned int LED_PIN = 13;
static const unsigned int GATE_DELAY = 10;

const int capacity = 100;
StaticJsonDocument<capacity> doc;

enum gate_state {
  raised, raising,
  lowered, lowering
};

gate_state state = lowered;

void send_current_state() {
  DynamicJsonDocument dyn_doc(100);
  dyn_doc[TYPE] = QUERY_STATE_RESULT;

  const char* state_str = nullptr;
  if (state == raised)    state_str = STATE_RAISED;
  if (state == raising)   state_str = STATE_RAISING;
  if (state == lowered)   state_str = STATE_LOWERED;
  if (state == lowering)  state_str = STATE_LOWERING;

  dyn_doc[PAYLOAD] = state_str;

  String msg;
  serializeJson(dyn_doc, msg);

  Serial.write(msg.c_str());
}

void change_state(gate_state new_state, bool persist_change = true) {
  if (persist_change) {
    state = new_state;
    send_current_state();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  pinMode(LED_PIN, OUTPUT);

  change_state(lowered);
}

void raise_gate() {
  change_state(raising);
  
  unsigned char progress = 0;
  while (progress < 255) {
    analogWrite(LED_PIN, progress);
    progress++;
    delay(GATE_DELAY);
  }

  // flush/ignore any incoming messages
  Serial.readString();

  change_state(raised);
}

void lower_gate() {
  change_state(lowering);
  
  unsigned char progress = 255;
  while (progress > 0) {
    analogWrite(LED_PIN, progress);
    progress--;
    delay(GATE_DELAY);
  }

  // flush/ignore any incoming messages
  Serial.readString();

  change_state(lowered);
}

void loop() {
  if (Serial.available() == 0) {
    return;
  }

  String msg = Serial.readString();

  deserializeJson(doc, msg);

  if (doc[TYPE] == CHANGE_STATE) {
    if (doc[PAYLOAD] == true) {
      if (state != raised) raise_gate();
    } else if (doc[PAYLOAD] == false) {
      if (state != lowered) lower_gate();
    }
  } else if (doc[TYPE] == QUERY_STATE) {
    send_current_state();
  } else {
    Serial.write(ERROR_MESSAGE);
  }
}
