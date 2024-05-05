#ifndef RHEOSCAPE_MOTOR_DRIVER_H
#define RHEOSCAPE_MOTOR_DRIVER_H

#ifdef PLATFORM_ARDUINO

#include <input/Input.h>
#include <output/Output.h>

class MotorDriver : public Output {
  private:
    uint8_t _forwardPin;
    uint8_t _backwardPin;
    uint8_t _pwmPin;
    // Does setting a pin HIGH activate it or deactivate it?
    bool _controlPinActiveState;
    Input<float>* _input;

  public:
    MotorDriver(uint8_t forwardPin, uint8_t backwardPin, uint8_t pwmPin, bool controlPinActiveState, Input<float>* input)
    : _forwardPin(forwardPin),
      _backwardPin(backwardPin),
      _pwmPin(pwmPin),
      _controlPinActiveState(controlPinActiveState),
      _input(input)
    {
      pinMode(_forwardPin, OUTPUT);
      pinMode(_backwardPin, OUTPUT);
      if (_pwmPin > 0) {
        pinMode(_pwmPin, OUTPUT);
      }
    }

    virtual void run() {
      std::optional<float> value = _input->read();
      if (!value.has_value()) {
        return;
      }

      if (_pwmPin > 0) {
        // Normalise -1...0 to 0...1023 and 0...1 to 0...1023 too, to drive PWM pins.
        // Positive vs negative happens in the next step.
        int pwmValue = round(abs(value.value()) * 1023);
        analogWrite(_pwmPin, pwmValue);
      }

      if (value < 0) {
        digitalWrite(_forwardPin, !_controlPinActiveState);
        digitalWrite(_backwardPin, _controlPinActiveState);
      } else if (value > 0) {
        digitalWrite(_forwardPin, _controlPinActiveState);
        digitalWrite(_backwardPin, !_controlPinActiveState);
      } else {
        digitalWrite(_forwardPin, !_controlPinActiveState);
        digitalWrite(_backwardPin, !_controlPinActiveState);
      }
    }
};

#endif

#endif