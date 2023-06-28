#ifndef RHEOSCAPE_OUTPUT_H
#define RHEOSCAPE_OUTPUT_H

#include <optional>
#include <Arduino.h>
#include <twilio.hpp>

#include <Process.h>
#include <Range.h>
#include <Runnable.h>
#include <Timer.h>

// An output is not a class; it's just a pattern of taking an input and implementing a run() function,
// then reacting to the input when appropriate within run().

// Zero-crossing SSRs, while fast, can create weird bias voltages if you switch them too fast.
#define SOLID_STATE_RELAY_CYCLE_TIME 100
// Don't thrash the poor mechanical relays.
#define MECHANICAL_RELAY_CYCLE_TIME 2000
// Be even gentler with things that have inductive loads in them.
#define INDUCTIVE_LOAD_CYCLE_TIME 1000 * 60

class Output : public Runnable {
  public:
    Output(Runner runner) : Runnable(runner) { }
};

class DigitalPinOutput : public Output {
  private:
    // Does this relay go on when its pin is HIGH or LOW?
    bool _onState;
    uint8_t _pin;
    Input<bool>& _input;

  public:
    DigitalPinOutput(uint8_t pin, bool onState, Input<bool> input, Runner runner)
    :
      _pin(pin),
      _onState(onState),
      _input(input),
      Output(runner)
    {
      pinMode(_pin, OUTPUT);
    }

    void run() {
      digitalWrite(_pin, _input.read() ? _onState : !_onState);
    }
};

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

class MotorDriver : public Output {
  private:
    uint8_t _forwardPin;
    uint8_t _backwardPin;
    uint8_t _pwmPin;
    // Does setting a pin HIGH activate it or deactivate it?
    bool _controlPinActiveState;
    Input<float> _input;

  public:
    MotorDriver(uint8_t forwardPin, uint8_t backwardPin, uint8_t pwmPin, bool controlPinActiveState, Input<float> input, Runner runner)
    : _forwardPin(forwardPin),
      _backwardPin(backwardPin),
      _pwmPin(pwmPin),
      _controlPinActiveState(controlPinActiveState),
      _input(input),
      Output(runner)
    {
      pinMode(_forwardPin, OUTPUT);
      pinMode(_backwardPin, OUTPUT);
      if (_pwmPin > 0) {
        pinMode(_pwmPin, OUTPUT);
      }
    }

    void run() {
      std::optional<float> value = _input.read();
      if (!value.has_value()) {
        return;
      }

      if (_pwmPin > 0) {
        // Normalise -1...0 to 0...255 and 0...1 to 0...255 too, to drive PWM pins.
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

// Wrap the input in a throttling process, to be gentle on hardware.
// Mechanical relays have coils that can't take more than e.g. a 2 second cycling time,
// zero-crossing solid-state relays can create weird dirty power problems
// if their cycle time doesn't line up perfectly with the mains cycle,
// and some loads (e.g., inductive loads) can't take fast cycling on an AC supply either.
Output makeRelay(
  uint8_t pin,
  bool onState,
  unsigned long minCycleTime,
  Input<bool> input,
  Runner runner
) {
  Input<bool> throttle = ThrottlingProcess<bool>(
    input,
    minCycleTime
  );
  return DigitalPinOutput(
    pin,
    onState,
    throttle,
    runner
  );
}

// Wrap a motor controller in a throttling process that also converts booleans to floats.
// The excursion time should allow enough time for the cover to fully open or close.
// TODO: Maybe limit switches could be used for a more advanced cover in the future.
Output makeCover(
  uint8_t forwardPin,
  uint8_t backwardPin,
  uint8_t pwmPin,
  bool controlPinActiveState,
  unsigned long excursionTime,
  Input<bool> input,
  Runner runner
) {
  Input<bool> throttle = ThrottlingProcess<bool>(
    input,
    excursionTime
  );
  Input<float> translator = TranslatingProcess<bool, float>(
    throttle,
    [](std::optional<bool> value) {
      if (value.has_value()) {
        return (std::optional<float>)(value ? 1.0 : -1.0);
      }
      return (std::optional<float>)std::nullopt;
    }
  );
  return MotorDriver(
    forwardPin,
    backwardPin,
    pwmPin,
    controlPinActiveState,
    translator,
    runner
  );
}

struct TwilioConfig {
  std::string accountId;
  std::string authToken;
  std::string sender;
  std::string recipient;
};

// Send a message via Twilio every n milliseconds,
// unless the message is blank.
// TODO: make the config an input?
// WARNING: There's no error checking whatsoever in here.
// And in fact there's no way in the entire framework to catch errors.
// TODO: make a logger.
class TwilioMessageOutput : public Output {
  private:
    std::optional<std::string> _lastMessageSent;
    Input<std::string> _input;
    Throttle _throttle;
    TwilioConfig _config;
    Twilio _client;

    void _sendMessage(std::optional<std::string> message) {
      if (message == _lastMessageSent) {
        return;
      }

      if (!message.has_value()) {
        _lastMessageSent = message;
        return;
      }

      String response;
      _client.send_message(
        _config.recipient.c_str(),
        _config.sender.c_str(),
        message.value().c_str(),
        response
      );
      _lastMessageSent = message;
    }
  
  public:
    TwilioMessageOutput(Input<std::string> input, unsigned long sendEvery, TwilioConfig config, Runner runner)
    :
      _input(input),
      _throttle(Throttle(
        sendEvery,
        [this]() {
          _sendMessage(_input.read());
        }
      )),
      _config(config),
      _client(config.accountId.c_str(), config.authToken.c_str()),
      Output(runner)
    { }

    void run() {
      _throttle.tryRun();
    }
};

#endif