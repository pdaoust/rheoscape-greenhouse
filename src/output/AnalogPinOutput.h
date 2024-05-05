#ifndef RHEOSCAPE_ANALOG_PIN_OUTPUT_H
#define RHEOSCAPE_ANALOG_PIN_OUTPUT_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>
#include <output/Output.h>

class AnalogPinOutput : public Output {
  private:
    uint8_t _pin;
    Input<float>* _input;

  public:
    AnalogPinOutput(uint8_t pin, Input<float>* input)
    :
      _pin(pin),
      _input(input)
    {
      pinMode(_pin, OUTPUT);
    }

    virtual void run() {
      analogWrite(_pin, _input->read() * 255);
    }
};

#endif

#endif