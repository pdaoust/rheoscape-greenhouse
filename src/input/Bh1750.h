#ifndef RHEOSCAPE_BH1750_H
#define RHEOSCAPE_BH1750_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <Timer.h>
#include <input/Input.h>
#include <Wire.h>
#include <BH1750.h>

class Bh1750 : public Input<float> {
  private:
    BH1750 _lightMeter;
    Timer _timer;
    float _lastReadValue;

  public:
    Bh1750(unsigned long sampleInterval, uint8_t address = 0x23)
    :
      _lightMeter(BH1750(address)),
      _timer(Timer(
        sampleInterval,
        [this]() {
          if (_lightMeter.measurementReady()) {
            _lastReadValue = _lightMeter.readLightLevel();
          }
        },
        std::nullopt,
        true
      ))
    {
      Wire.begin();
      _lightMeter.begin();
    }

    virtual float read() {
      _timer.run();
      return _lastReadValue;
    }
};

#endif

#endif