#ifndef RHEOSCAPE_ANALOG_PIN_OUTPUT_H
#define RHEOSCAPE_ANALOG_PIN_OUTPUT_H

#ifdef PLATFORM_ARDUINO

#include <output/Output.h>

class AnalogPinOutput : public Output {
  private:
    uint8_t _pin;
    Input<float> _input;

  public:
    AnalogPinOutput(uint8_t pin, Input<float> input, Runner runner)
    :
      _pin(pin),
      _input(input),
      Output(runner)
    {
      pinMode(_pin, OUTPUT);
    }

    void run() {
      std::optional<float> value = _input.read();
      if (value.has_value()) {
        analogWrite(_pin, value.value() * 255);
      }
    }
};

#endif

#endif