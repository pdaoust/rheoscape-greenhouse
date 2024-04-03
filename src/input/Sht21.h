#ifndef RHEOSCAPE_SHT21_H
#define RHEOSCAPE_SHT21_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>
#include <SHT2x.h>

enum class Sht21Channel {
  tempC,
  humidity
};

class Sht21 : public MultiInput<Sht21Channel, std::optional<float>> {
  private:
    SHT21 _input;
    float _tempOffset;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadHum;
    // What are we currently polling for?
    // Maps to the underlying library's request type:
    // 0 = no polling happening
    // 1 = polling for temperature
    // 2 = polling for humidity
    uint8_t _mode = 0;

  public:
    Sht21(uint8_t dataPin, uint8_t clockPin, float tempOffset = 0.0)
    :
      _input(SHT21()),
      _tempOffset(tempOffset)
    {
      _input.begin(dataPin, clockPin);
    }

    std::optional<float> readChannel(Sht21Channel channel) {
      switch (_mode) {
        case 0:
          // Not polling for anything. Start the poll on the requested channel.
          switch (channel) {
            case Sht21Channel::tempC:
              _input.requestTemperature();
              _mode = 1;
              // And return the last known value.
              return _lastReadTemp;
            case Sht21Channel::humidity:
              _input.requestHumidity();
              _mode = 2;
              return _lastReadHum;
            // Unreachable; doing it to shut the compiler up.
            default: return std::nullopt;
          }
        case 1:
          // Temp has been requested and we're currently polling for it.
          switch (channel) {
            case Sht21Channel::tempC:
              // Once it's ready and there are no errors,
              // get the temp and reset the polling mode so we know we can poll again.
              if (_input.reqTempReady() && _input.readTemperature()) {
                _lastReadTemp = _input.getTemperature();
                _mode = 0;
              }
              return _lastReadTemp;
            case Sht21Channel::humidity:
              // not currently polling for humidity; just return the last known value.
              return _lastReadHum;
            default: return std::nullopt;
          }
        case 2:
          switch (channel) {
            case Sht21Channel::humidity:
              if (_input.reqHumReady() && _input.readHumidity()) {
                _lastReadHum = _input.getHumidity();
                _mode = 0;
              }
              return _lastReadHum;
            case Sht21Channel::tempC:
              return _lastReadTemp;
            default: return std::nullopt;
          }
        default: return std::nullopt;
      }
    }
};

#endif

#endif