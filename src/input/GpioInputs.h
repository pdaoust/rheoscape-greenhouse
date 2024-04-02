#ifndef RHEOSCAPE_GPIO_INPUTS_H
#define RHEOSCAPE_GPIO_INPUTS_H

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

    bool read() {
      bool pinState = digitalRead(_pin);
      return _onState ? pinState : !pinState;
    }
};

class AnalogPinInput : public Input<float> {
  private:
    uint8_t _pin;
    static uint8_t _resolution;
    static bool _resolutionSetOnce;

  public:
    AnalogPinInput(uint8_t pin)
    : _pin(pin)
    {
      if (!_resolutionSetOnce) {
        setResolution(10);
        _resolutionSetOnce = true;
      }
    }

    float read() {
      return (float)analogRead(_pin) / (2 ^ _resolution - 1);
    }

    static void setResolution(uint8_t resolution) {
      _resolution = resolution;
      _resolutionSetOnce = true;
      analogReadResolution(resolution);
    }
};

#endif