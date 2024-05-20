#include <string>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Range.h>
#include <input/Input.h>
#include <GreenhouseState.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

enum WebSocketMessageType {
  stateUpdated,
};

template <typename T>
struct WebSocketMessage {
  WebSocketMessageType type;
  T data;
};

template <typename T>
void sendWebSocketMessage(WebSocketMessage<T> message) {
  JsonObject message;
  message["type"] = message.type;
  message["data"] = message.data;
  AsyncWebSocketMessageBuffer* buffer = ws.makeBuffer(len);
  if (buffer) {
    doc.printTo((char*)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
}

void sendStateUpdatedMessages(std::map<std::string, JsonDocument> messages) {
  JsonObject data;
  for (std::pair<std::string, JsonDocument> const kvp : messages) {
    data[kvp.first] = kvp.second;
  }
  WebSocketMessage<JsonObject> message = { WebSocketMessageType::stateUpdated, data };
  sendWebSocketMessage(message);
}

template <typename T>
void sendStateUpdatedMessage(std::string key, T value) {
  JsonDocument valueAsJson;
  valueAsJson.set(value);
  std::map<std::string, JsonDocument> obj;
  obj[key] = valueAsJson;
  sendStateUpdatedMessages(obj);
}

template <typename T>
void convertToJson(const Range<T>& value, JsonVariant dest) {
  JsonObject valueAsJson;
  valueAsJson["min"] = value.min;
  valueAsJson["max"] = value.max;
  dest.set(valueAsJson);
}

template <typename T>
void convertToJson(const SetpointAndHysteresis<T>& value, JsonVariant dest) {
  JsonObject valueAsJson;
  valueAsJson["setpoint"] = value.setpoint;
  valueAsJson["hysteresis"] = value.hysteresis;
  dest.set(valueAsJson);
}

void receiveWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    uint8_t* messageData;
    AwsFrameInfo* info = (AwsFrameInfo*) arg;

    if (info->final && info->index == 0 && info->len == len) {
      // The whole message is in a single frame and we got all of its data.
      data[len] = 0;
      messageData = data;
    } else {
      // Given that a frame can theoretically be 9 exabytes long,
      // and I don't imagine I'll ever try to stream fragmented frames,
      // we're not going to try to figure out how to compile multiple frames into one byte array.
    }

    JsonDocument messageJson;
    deserializeJson(messageJson, messageData);

    if (strcmp(messageJson["type"], "setState")) {
      GreenhouseState state = GreenhouseState::getState();
      JsonObject setStateData = messageJson["data"];
      for (JsonPair kvp : setStateData) {
        std::string stateToUpdate = std::string(kvp.key().c_str());
        if (stateToUpdate == "temp-unit") {
          std::string newTempUnit = kvp.value().as<std::string>();
          if (newTempUnit == "celsius") {
            state.temp_unit->write(TempUnit::celsius);
          } else if (newTempUnit == "fahrenheit") {
            state.temp_unit->write(TempUnit::fahrenheit);
          } else if (newTempUnit == "kelvin") {
            state.temp_unit->write(TempUnit::kelvin);
          }
        } else if (stateToUpdate == "fan-control-setpoint") {
          state.fan_control->write(SetpointAndHysteresis(kvp.value().as<float>(), state.fan_control->read().hysteresis));
        } else if (stateToUpdate == "fan-control-hysteresis") {
          state.fan_control->write(SetpointAndHysteresis(state.fan_control->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "heater-control-setpoint") {
          state.heater_control->write(SetpointAndHysteresis(kvp.value().as<float>(), state.heater_control->read().hysteresis));
        } else if (stateToUpdate == "heater-control-hysteresis") {
          state.heater_control->write(SetpointAndHysteresis(state.heater_control->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "door-alarm-control-min") {
          state.door_alarm_control->write(Range(kvp.value().as<float>(), state.door_alarm_control->read().max));
        } else if (stateToUpdate == "door-alarm-control-max") {
          state.door_alarm_control->write(Range(state.door_alarm_control->read().min, kvp.value().as<float>()));
        } else if (stateToUpdate == "roof-vents-control-setpoint") {
          state.roof_vents_control->write(SetpointAndHysteresis(kvp.value().as<float>(), state.roof_vents_control->read().hysteresis));
        } else if (stateToUpdate == "roof-vents-control-hysteresis") {
          state.roof_vents_control->write(SetpointAndHysteresis(state.roof_vents_control->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "mat-1-control-setpoint") {
          state.mat_1_control->write(SetpointAndHysteresis(kvp.value().as<float>(), state.mat_1_control->read().hysteresis));
        } else if (stateToUpdate == "mat-1-control-hysteresis") {
          state.mat_1_control->write(SetpointAndHysteresis(state.mat_1_control->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "mat-2-control-setpoint") {
          state.mat_2_control->write(SetpointAndHysteresis(kvp.value().as<float>(), state.mat_2_control->read().hysteresis));
        } else if (stateToUpdate == "mat-2-control-hysteresis") {
          state.mat_2_control->write(SetpointAndHysteresis(state.mat_2_control->read().setpoint, kvp.value().as<float>()));
        }
      }
    }
  }
}

void setupWebServer() {
  GreenhouseState state = GreenhouseState::getState();
  state.temp_unit->registerSubscriber([](Event<TempUnit> v) {
    std::string unitSymbol;
    switch (v.value) {
      case TempUnit::celsius: unitSymbol = "°C"; break;
      case TempUnit::fahrenheit: unitSymbol = "°F"; break;
      case TempUnit::kelvin: unitSymbol = "K"; break;
    }
    sendStateUpdatedMessage("temp-unit", unitSymbol);
  });
  ws.onEvent(receiveWebSocketEvent);
  state.shelf_temp->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("shelf-temp", e.value); });
  state.shelf_hum->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("shelf-hum", e.value); });
  state.shelf_light->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("shelf-light", e.value); });
  state.ground_temp->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("ground-temp", e.value); });
  state.ceil_temp->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("ceil-temp", e.value); });
  state.yuzu_temp->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("yuzu-temp", e.value); });
  state.fan_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("fan-status", e.value ? "on" : "off"); });
  state.fan_control->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("fan-control", e.value); });
  state.heater_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("heater-status", e.value ? "on" : "off"); });
  state.heater_control->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("heater-control", e.value); });
  state.west_door_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("west-door-status", e.value ? "closed" : "open"); });
  state.east_door_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("east-door-status", e.value ? "closed" : "open"); });
  state.door_alarm_control->registerSubscriber([](Event<Range<float>> e) { sendStateUpdatedMessage("door-alarm-control", e.value); });
  state.door_alarm_noise->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("door-alarm-nose", e.value); });
  state.door_alarm_phone->registerSubscriber([](Event<std::string> e) { sendStateUpdatedMessage("door-alarm-phone", e.value); });
  state.roof_vents_control->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("roof-vents-control", e.value); });
  state.mat_1_control->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("mat-1-control", e.value); });
  state.mat_2_control->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("mat-2-control", e.value); });
  server.addHandler(&ws);
}