#ifndef RHEOSCAPE_MAX6675_H
#define RHEOSCAPE_MAX6675_H
#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <MAX6675.h>

#include <Timer.h>
#include <input/Input.h>

class Max6675 : public Input<std::optional<float>> {
  private:
    MAX6675 _sensor;
    Throttle _throttle;
    std::optional<float> _lastReading;
  
  public:
    Max6675(uint8_t csPin, SPIClass* spi)
    :
      _sensor(MAX6675(csPin, spi)),
      // The MAX6675 only updates its internal reading every 0.22 seconds.
      _throttle(220, [this]() {
        if (_sensor.read() != STATUS_OK) {
          _lastReading = std::nullopt;
        }
        _lastReading = _sensor.getTemperature();
      })
    { }

    virtual std::optional<float> read() {
      _throttle.tryRun();
      return _lastReading;
    }
};

#endif
#endif