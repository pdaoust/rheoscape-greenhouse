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
    SHT21 _sensor;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadHum;

    // What are we currently polling for?
    // Maps to the underlying library's request type:
    // 0 = no polling happening
    // 1 = polling for temperature
    // 2 = polling for humidity
    enum _SensorMode {
      NotPolling = 0,
      PollingTemp = 1,
      PollingHum = 2
    };

    _SensorMode _mode;

  public:
    Sht21(TwoWire* i2c)
    :
      _mode(_SensorMode::NotPolling),
      _sensor(SHT21())
    {
      _sensor.begin(i2c);
    }

    std::optional<float> readChannel(Sht21Channel channel) {
      switch (_mode) {
        case _SensorMode::NotPolling:
          // Not polling for anything. Start the poll on the requested channel.
          switch (channel) {
            case Sht21Channel::tempC:
              _sensor.requestTemperature();
              _mode = _SensorMode::PollingTemp;
              // And return the last known value.
              return _lastReadTemp;
            case Sht21Channel::humidity:
              _sensor.requestHumidity();
              _mode = _SensorMode::PollingHum;
              return _lastReadHum;
            // Unreachable; doing it to shut the compiler up.
            default: return std::nullopt;
          }
        case _SensorMode::PollingTemp:
          // Temp has been requested and we're currently polling for it.
          switch (channel) {
            case Sht21Channel::tempC:
              // Once it's ready and there are no errors,
              // get the temp and reset the polling mode so we know we can poll again.
              if (_sensor.reqTempReady() && _sensor.readTemperature()) {
                _lastReadTemp = _sensor.getTemperature();
                _mode = _SensorMode::NotPolling;
              }
              return _lastReadTemp;
            case Sht21Channel::humidity:
              // not currently polling for humidity; just return the last known value.
              return _lastReadHum;
            default: return std::nullopt;
          }
        case _SensorMode::PollingHum:
          switch (channel) {
            case Sht21Channel::humidity:
              if (_sensor.reqHumReady() && _sensor.readHumidity()) {
                _lastReadHum = _sensor.getHumidity();
                _mode = _SensorMode::NotPolling;
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