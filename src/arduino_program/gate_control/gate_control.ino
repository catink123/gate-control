#include <ArduinoJson.h>
#include <string.h>
#include <math.h>

const char* TYPE = "type";
const char* PAYLOAD = "payload";
const char* CHANGE_STATE = "change_state";
const char* QUERY_STATE = "query_state";
const char* QUERY_STATE_RESULT = "query_state_result";
const char* CONFIG = "config";

const char* ERROR_MESSAGE_START = "{\"type\": \"error\", \"payload\": \"";
const char* ERROR_MESSAGE_END = "\"}";

const char* STATE_RAISED = "raised";
const char* STATE_RAISING = "raising";
const char* STATE_LOWERED = "lowered";
const char* STATE_LOWERING = "lowering";

const unsigned int LED_PIN = 13;
const unsigned int GATE_DELAY = 10;

const unsigned int GATE_PINS[] = { 3, 5, 6, 9, 10, 11, 13 };
const unsigned int GATE_PINS_SIZE = 7;

JsonDocument doc;

enum GateState {
  Raised, Raising,
  Lowered, Lowering
};

class Gate {
  static const unsigned int CHANGE_DELAY = 10;
  
  bool just_finished = false;
  bool is_lowering = false;
  bool is_changing = false;

public:
  float state = 0.0F;

  // returns true if state changed
  bool tick() {
    if (just_finished) {
      just_finished = false;
    }
    if (!is_changing) {
      return false;
    }

    if (is_lowering) {
      state -= 0.01F;
    } else {
      state += 0.01F;
    }

    delay(CHANGE_DELAY);

    if (state <= 0.0F || state >= 1.0F) {
      is_changing = false;
      just_finished = true;

      if (state < 0.0F) {
        state = 0.0F;
      } else if (state > 1.0F) {
        state = 1.0F;
      }
    }

    return true;
  }

  bool finished_moving() const {
    return just_finished;
  }

  void lower() {
    is_lowering = true;
    is_changing = true;
  }

  void raise() {
    is_lowering = false;
    is_changing = true;
  }

  GateState get_state() const {
    if (!is_changing) {
      if (state == 1.0F) return Raised;
      if (state == 0.0F) return Lowered;
    } else {
      if (is_lowering)  return Lowering;
      else              return Raising;
    }
  }
};

Gate gates[GATE_PINS_SIZE];

void send_gate_states(unsigned int* ids, unsigned int ids_count) {
  JsonDocument dyn_doc;
  dyn_doc[TYPE] = QUERY_STATE_RESULT;

  JsonArray payload_arr = dyn_doc[PAYLOAD].to<JsonArray>();

  for (int i = 0; i < ids_count; i++) {
    unsigned int& id = ids[i];
    if (id >= GATE_PINS_SIZE) {
      continue;
    }

    const char* state_str = nullptr;
    GateState state = gates[id].get_state();
    if (state == Raised)    state_str = STATE_RAISED;
    if (state == Raising)   state_str = STATE_RAISING;
    if (state == Lowered)   state_str = STATE_LOWERED;
    if (state == Lowering)  state_str = STATE_LOWERING;

    JsonObject obj = payload_arr.add<JsonObject>();

    obj["id"] = id;
    obj["state"] = state_str;
  }

  serializeJson(dyn_doc, Serial);
}

void send_gate_states(JsonArray ids) {
  JsonDocument dyn_doc;
  dyn_doc[TYPE] = QUERY_STATE_RESULT;

  JsonArray payload_arr = dyn_doc[PAYLOAD].to<JsonArray>();

  for (JsonVariant el : ids) {
    if (!el.is<unsigned int>()) {
      continue;
    }
    unsigned int id = el.as<unsigned int>();
    if (id >= GATE_PINS_SIZE) {
      continue;
    }

    const char* state_str = nullptr;
    GateState state = gates[id].get_state();
    if (state == Raised)    state_str = STATE_RAISED;
    if (state == Raising)   state_str = STATE_RAISING;
    if (state == Lowered)   state_str = STATE_LOWERED;
    if (state == Lowering)  state_str = STATE_LOWERING;

    JsonObject obj = payload_arr.add<JsonObject>();

    obj["id"] = id;
    obj["state"] = state_str;
  }

  serializeJson(dyn_doc, Serial);
}

void send_gate_state(unsigned int id) {
  unsigned int arr[1] = { id };
  send_gate_states(arr, 1);
}

unsigned int index_to_pin(unsigned int id) {
  if (id >= GATE_PINS_SIZE) {
    return GATE_PINS_SIZE + 1;
  }
  return GATE_PINS[id];
}

void send_error(const char* payload = "unknown") {
  int new_str_size = strlen(ERROR_MESSAGE_START) + strlen(payload) + strlen(ERROR_MESSAGE_END);
  char* new_str = new char[new_str_size];
  for (int i = 0; i < new_str_size; i++) {
    new_str[i] = '\0';
  }

  strcat(new_str, ERROR_MESSAGE_START);
  strcat(new_str, payload);
  strcat(new_str, ERROR_MESSAGE_END);

  Serial.write(new_str);

  delete[] new_str;
}

void handle_message() {
  // skip null messages
  if (doc.isNull()) {
    return;
  }

  if (!doc[TYPE].is<const char*>()) {
    send_error("malformed_type");
    return;
  }
  const char* type = doc[TYPE].as<const char*>();

  if (strcmp(type, CHANGE_STATE) == 0) {
    if (!doc[PAYLOAD].is<JsonObject>()) {
      send_error("malformed_change_state_payload");
      return;
    }
    JsonObject msg_data = doc[PAYLOAD].as<JsonObject>();

    if (!msg_data["id"].is<unsigned int>() || !msg_data["state"].is<bool>()) {
      send_error("malformed_change_state_payload");
      return;
    }

    unsigned int id = msg_data["id"].as<unsigned int>();
    bool new_state = msg_data["state"].as<bool>();

    Gate& selected_gate = gates[id];

    if (new_state) {
      selected_gate.raise();
      send_gate_state(id);
    } else {
      selected_gate.lower();
      send_gate_state(id);
    }
  } else if (strcmp(type, QUERY_STATE) == 0) {
    if (!doc[PAYLOAD].is<JsonArray>()) {
      send_error("malformed_query_state_payload");
      return;
    }
    JsonArray query = doc[PAYLOAD].as<JsonArray>();

    send_gate_states(query);
  } else {
    send_error();
  }
}

void update_gates() {
  for (int i = 0; i < GATE_PINS_SIZE; i++) {
    Gate& gate = gates[i];

    // if the tick changed state
    if (gate.tick()) {
      // update physical LED state
      unsigned int pin = GATE_PINS[i];
      unsigned char value = static_cast<unsigned char>(floorf(gate.state * 255));
      analogWrite(pin, value);
    }

    if (gate.finished_moving()) {
      send_gate_state(i);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < GATE_PINS_SIZE; i++) {
    pinMode(GATE_PINS[i], OUTPUT);
  }
}

void loop() {
  if (Serial.available() > 0) {
    deserializeJson(doc, Serial);
    handle_message();
  }

  update_gates();
}
