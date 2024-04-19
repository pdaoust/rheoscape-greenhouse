#ifndef RHEOSCAPE_GPIO_INPUTS_H
#define RHEOSCAPE_GPIO_INPUTS_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>

class DigitalPinInput : public Input<bool> {
  private:
    uint8_t _pin;
    bool _onState;
  
  public:
    DigitalPinInput(uint8_t pin, uint8_t mode)
    :
      _pin(pin),
      _onState(mode == INPUT_PULLDOWN)
    {
      pinMode(_pin, mode);
    }

    virtual bool read() {
      bool pinState = digitalRead(_pin);
      return _onState ? pinState : !pinState;
    }
};

class AnalogPinInput : public Input<float> {
  private:
    uint8_t _pin;
    uint8_t _resolution;
    static bool _resolutionSetOnce;

  public:
    AnalogPinInput(uint8_t pin, uint8_t resolution = 10)
    : _pin(pin)
    {
      if (!_resolutionSetOnce) {
        analogReadResolution(16);
        _resolutionSetOnce = true;
      }
    }

    virtual float read() {
      return (float)analogRead(_pin) / (2 ^ _resolution - 1);
    }
};

#endif

#endif