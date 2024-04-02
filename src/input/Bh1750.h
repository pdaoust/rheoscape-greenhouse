#ifndef RHEOSCAPE_BH1750_H
#define RHEOSCAPE_BH1750_H

#include <Arduino.h>
#include <input/Input.h>
#include <Wire.h>
#include <BH1750.h>

class Bh1750 : public Input<std::optional<float>> {
  private:
    BH1750 _lightMeter;
    RepeatTimer _timer;
    std::optional<float> _lastReadValue;

  public:
    Bh1750(uint8_t address, unsigned long sampleInterval)
    :
      _lightMeter(BH1750(address)),
      _timer(RepeatTimer(
        millis(),
        sampleInterval,
        [this]() {
          if (_lightMeter.measurementReady()) {
            _lastReadValue = _lightMeter.readLightLevel();
          }
          return _lastReadValue;
        }
      ))
    {
      Wire.begin();
      _lightMeter.begin();
    }

    std::optional<float> read() {
      _timer.tick();
      return _lastReadValue;
    }
};

#endif