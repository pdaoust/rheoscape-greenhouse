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
    static const uint8_t BH1750_ADDRESS_LOW = 0x23;
    static const uint8_t BH1750_ADDRESS_HIGH = 0x5c;

    Bh1750(const unsigned long sampleInterval, const uint8_t address, TwoWire* i2c)
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
      _lightMeter.begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, address, i2c);
    }

    virtual float read() {
      _timer.run();
      return _lastReadValue;
    }
};

#endif

#endif