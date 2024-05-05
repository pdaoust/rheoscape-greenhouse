#ifndef RHEOSCAPE_DIGITAL_PIN_OUTPUT_H
#define RHEOSCAPE_DIGITAL_PIN_OUTPUT_H

#ifdef PLATFORM_ARDUINO

#include <input/Input.h>
#include <output/Output.h>

class DigitalPinOutput : public Output {
  private:
    // Does this relay go on when its pin is HIGH or LOW?
    bool _onState;
    uint8_t _pin;
    Input<bool>* _input;

  public:
    DigitalPinOutput(uint8_t pin, bool onState, Input<bool>* input)
    :
      _pin(pin),
      _onState(onState),
      _input(input)
    {
      pinMode(_pin, OUTPUT);
    }

    virtual void run() {
      digitalWrite(_pin, _input->read() ? _onState : !_onState);
    }
};

#endif

#endif