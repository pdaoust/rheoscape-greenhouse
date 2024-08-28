#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Range.h>
#include <helpers/string_format.h>
#include <input/Input.h>
#include <input/GpioInputs.h>
#include <output/OutputFactories.h>
#include <GreenhouseState.h>
#include <JsonConverters.h>

extern const uint8_t src_greenhouse_index_html_start[] asm("_binary_src_greenhouse_index_html_start");
extern const uint8_t src_greenhouse_index_html_end[]   asm("_binary_src_greenhouse_index_html_end");

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
  JsonObject jsonDoc;
  jsonDoc["type"] = message.type;
  jsonDoc["data"] = message.data;
  size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer* buffer = ws.makeBuffer(len);
  if (buffer) {
    serializeJson(jsonDoc, (char*)buffer->get(), len);
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

void updateTwoPointCalibrationValue(StateInput<TwoPointCalibration<float>>* input, std::string prop, float newPropValue) {
  TwoPointCalibration<float> oldValue = input->read();
  input->write(TwoPointCalibration(
    prop == "low_reference" ? newPropValue : oldValue.lowReference,
    prop == "low_raw" ? newPropValue : oldValue.lowRaw,
    prop == "high_reference" ? newPropValue : oldValue.highReference,
    prop == "high_raw" ? newPropValue : oldValue.highRaw
  ));
}

void receiveWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len, GreenhouseState* ghState) {
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
      JsonObject setStateData = messageJson["data"];
      for (JsonPair kvp : setStateData) {
        std::string stateToUpdate = std::string(kvp.key().c_str());
        if (stateToUpdate == "temp_unit") {
          std::string newTempUnit = kvp.value().as<std::string>();
          if (newTempUnit == "celsius") {
            ghState->temp_unit->write(TempUnit::celsius);
          } else if (newTempUnit == "fahrenheit") {
            ghState->temp_unit->write(TempUnit::fahrenheit);
          } else if (newTempUnit == "kelvin") {
            ghState->temp_unit->write(TempUnit::kelvin);
          }
        } else if (stateToUpdate == "fan_control_setpoint") {
          ghState->fan->write(SetpointAndHysteresis(kvp.value().as<float>(), ghState->fan->read().hysteresis));
        } else if (stateToUpdate == "fan_control_hysteresis") {
          ghState->fan->write(SetpointAndHysteresis(ghState->fan->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "heater_control_setpoint") {
          ghState->heater->write(SetpointAndHysteresis(kvp.value().as<float>(), ghState->heater->read().hysteresis));
        } else if (stateToUpdate == "heater_control_hysteresis") {
          ghState->heater->write(SetpointAndHysteresis(ghState->heater->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "extreme_temp_alarm_control_min") {
          ghState->extreme_temp_alarm_control->write(Range(kvp.value().as<float>(), ghState->extreme_temp_alarm_control->read().max));
        } else if (stateToUpdate == "extreme_temp_alarm_control_max") {
          ghState->extreme_temp_alarm_control->write(Range(ghState->extreme_temp_alarm_control->read().min, kvp.value().as<float>()));
        } else if (stateToUpdate == "door_alarm_control_min") {
          ghState->door_alarm_control->write(Range(kvp.value().as<float>(), ghState->door_alarm_control->read().max));
        } else if (stateToUpdate == "door_alarm_control_max") {
          ghState->door_alarm_control->write(Range(ghState->door_alarm_control->read().min, kvp.value().as<float>()));
        } else if (stateToUpdate == "roof_vents_control_setpoint") {
          ghState->roof_vents->write(SetpointAndHysteresis(kvp.value().as<float>(), ghState->roof_vents->read().hysteresis));
        } else if (stateToUpdate == "roof_vents_control_hysteresis") {
          ghState->roof_vents->write(SetpointAndHysteresis(ghState->roof_vents->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "mat_1_control_setpoint") {
          ghState->mat_1->write(SetpointAndHysteresis(kvp.value().as<float>(), ghState->mat_1->read().hysteresis));
        } else if (stateToUpdate == "mat_1_control_hysteresis") {
          ghState->mat_1->write(SetpointAndHysteresis(ghState->mat_1->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate == "mat_2_control_setpoint") {
          ghState->mat_2->write(SetpointAndHysteresis(kvp.value().as<float>(), ghState->mat_2->read().hysteresis));
        } else if (stateToUpdate == "mat_2_control_hysteresis") {
          ghState->mat_2->write(SetpointAndHysteresis(ghState->mat_2->read().setpoint, kvp.value().as<float>()));
        } else if (stateToUpdate.substr(0, 22) == "shelf_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->shelf_temp_calibration, stateToUpdate.substr(22), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 23) == "ground_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->ground_temp_calibration, stateToUpdate.substr(23), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 24) == "ceiling_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->ceiling_temp_calibration, stateToUpdate.substr(24), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 21) == "yuzu_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->yuzu_temp_calibration, stateToUpdate.substr(21), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 21) == "fish_tank_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->fish_tank_temp_calibration, stateToUpdate.substr(21), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 22) == "mat_1_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->mat_1_temp_calibration, stateToUpdate.substr(22), kvp.value().as<float>());
        } else if (stateToUpdate.substr(0, 22) == "mat_2_temp_calibration") {
          updateTwoPointCalibrationValue(ghState->mat_2_temp_calibration, stateToUpdate.substr(22), kvp.value().as<float>());
        }
      }
    }
  }
}

void setupWebServer(GreenhouseState* ghState) {
  ghState->temp_unit->registerSubscriber([](Event<TempUnit> v) { sendStateUpdatedMessage("temp_unit", v.value); });
  ws.onEvent([ghState](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) { receiveWebSocketEvent(server, client, type, arg, data, len, ghState); });
  ghState->shelf_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("shelf_temp", e.value); });
  ghState->shelf_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("shelf_temp_calibration", e.value); });
  ghState->shelf_hum->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("shelf_hum", e.value); });
  ghState->shelf_light->registerSubscriber([](Event<float> e) { sendStateUpdatedMessage("shelf_light", e.value); });
  ghState->ground_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("ground_temp", e.value); });
  ghState->ground_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("ground_temp_calibration", e.value); });
  ghState->ceiling_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("ceiling_temp", e.value); });
  ghState->ceiling_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("ceiling_temp_calibration", e.value); });
  ghState->yuzu_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("yuzu_temp", e.value); });
  ghState->yuzu_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("yuzu_temp_calibration", e.value); });
  ghState->fish_tank_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("fish_tank_temp", e.value); });
  ghState->fish_tank_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("fish_tank_temp_calibration", e.value); });
  ghState->fan_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("fan_status", e.value); });
  ghState->fan->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("fan", e.value); });
  ghState->heater_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("heater_status", e.value); });
  ghState->heater->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("heater", e.value); });
  ghState->west_door_status->registerSubscriber([](Event<DoorState> e) { sendStateUpdatedMessage("west_door_status", e.value); });
  ghState->east_door_status->registerSubscriber([](Event<DoorState> e) { sendStateUpdatedMessage("east_door_status", e.value); });
  ghState->extreme_temp_alarm_control->registerSubscriber([](Event<Range<float>> e) { sendStateUpdatedMessage("extreme_temp_alarm_control", e.value); });
  ghState->door_alarm_control->registerSubscriber([](Event<Range<float>> e) { sendStateUpdatedMessage("door_alarm_control", e.value); });
  ghState->alarm_noise->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("alarm_noise", e.value); });
  ghState->alarm_phone->registerSubscriber([](Event<std::string> e) { sendStateUpdatedMessage("alarm_phone", e.value); });
  ghState->roof_vents_status->registerSubscriber([](Event<CoverAction> e) { sendStateUpdatedMessage("roof_vents_status", e.value); });
  ghState->roof_vents_sensor_status->registerSubscriber([](Event<DoorState> e) { sendStateUpdatedMessage("roof_vents_sensor_status", e.value); });
  ghState->roof_vents->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("roof_vents", e.value); });
  ghState->mat_1_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("mat_1_status", e.value); });
  ghState->mat_1_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("mat_1_temp", e.value); });
  ghState->mat_1_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("mat_1_temp_calibration", e.value); });
  ghState->mat_1->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("mat_1", e.value); });
  ghState->mat_2_status->registerSubscriber([](Event<bool> e) { sendStateUpdatedMessage("mat_2_status", e.value); });
  ghState->mat_2_temp->registerSubscriber([](Event<std::optional<float>> e) { sendStateUpdatedMessage("mat_2_temp", e.value); });
  ghState->mat_2_temp_calibration->registerSubscriber([](Event<TwoPointCalibration<float>> e) { sendStateUpdatedMessage("mat_2_temp_calibration", e.value); });
  ghState->mat_2->registerSubscriber([](Event<SetpointAndHysteresis<float>> e) { sendStateUpdatedMessage("mat_2", e.value); });
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", String(src_greenhouse_index_html_start, src_greenhouse_index_html_end - src_greenhouse_index_html_start));
  });

  server.on("/allState", HTTP_GET, [ghState](AsyncWebServerRequest *request) {
    JsonDocument allStateJson;
    allStateJson.set(ghState);
    String buffer;
    serializeJson(allStateJson, buffer);
    request->send(200, "text/json", buffer);
  });
}

#endif