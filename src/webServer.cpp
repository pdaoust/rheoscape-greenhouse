#include <string>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

template <typename T>
void sendStateUpdated(std::string key, T value) {
  JsonDocument doc;
  doc["type"] = "stateUpdated";
  doc["key"] = key;
  doc["value"] = value;
  AsyncWebSocketMessageBuffer* buffer = ws.makeBuffer(len);
  if (buffer) {
    doc.printTo((char*)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
}