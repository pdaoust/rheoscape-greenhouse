#ifndef PAULDAOUST_CONTROLLER_H
#define PAULDAOUST_CONTROLLER_H

#include <Arduino.h>
#include <AsyncTimer.h>
#include <Range.h>

// Zero-crossing SSRs, while fast, can create weird bias voltages if you switch them too fast.
#define SOLID_STATE_RELAY_CYCLE_TIME 100
// Don't thrash the poor mechanical relays.
#define MECHANICAL_RELAY_CYCLE_TIME 2000
// Be even gentler with things that have inductive loads in them.
#define FAN_CYCLE_TIME 10000

template <typename T>
class Controller {
  public:
    virtual void setState(T state);
};

template <typename T>
class VariableController : public Controller<T> {
  public:
    virtual Range<T> getRange();
};

enum TriState {
  forward = 1,
  neutral = 0,
  backward = -1
};

template <typename T>
class ThrottledController : public Controller<T>, public Runnable {
  private:
    ThrottlingTimer _timer;
    Controller<T> _wrappedController;
    TOuter _outerState;

  public:
    ThrottledController(Controller<T> wrappedController, unsigned long minCycleTime)
    : _wrappedController(wrappedController),
      _timer(ThrottlingTimer(
        minCycleTime,
        [this]() {
          _wrappedController.setState(_outerState);
          _timer.reset();
        }
      ))
    { }

    void setState(T state) {
      run();
      _outerState = state;
    }

    void run() {
      _timer.run();
    }
};

template <typename TOuter, typename TInner>
class ControllerTranslator : Controller<TOuter> {
  private:
    Controller<TInner> _wrappedController;
    const std::function<TInner(TOuter)>& _translator;
  
  public:
    ControllerTranslator(Controller<TInner> wrappedController, const std::function<TInner(TOuter)>& translator)
    :
      _wrappedController(wrappedController),
      _translator(translator)
    { }

    void setState(TOuter state) {
      _wrappedController.setState(_translator(state));
    }
};

class TimedLatchController : public Controller<bool>, public Runnable {
  private:
    Controller<bool> _wrappedController;
    AsyncTimer _timeout;
  
  public:
    TimedLatchController(Controller<bool> wrappedController, unsigned long timeout)
    :
      _wrappedController(wrappedController),
      _timeout(AsyncTimer(
        timeout,
        2,
        [this]() {
          // On the first run, where count = 0, turn it on.
          // On the second run, turn it off.
          if (!_timeout.getCount()) {
            _wrappedController.setState(true);
          } else {
            _wrappedController.setState(false);
          }
        },
        -1
      ))
    { }

    void setState(bool state) {
      _timeout.reset(state ? 0 : -1);
    }
};

class BlinkingController : public Controller<bool>, public Runnable {
  private:
    AsyncTimer _fullCycle;
    TimedLatchController _onCycle;

  public:
    BlinkingController(Controller<bool> wrappedController, unsigned long onTime, unsigned long offTime)
    :
      _onCycle(TimedLatchController(
        wrappedController,
        onTime
      )),
      _fullCycle(AsyncTimer(
        onTime + offTime,
        0,
        [this]() {
          _onCycle.setState(true);
        },
        -1
      ))
    { }

    void setState(bool state) {
      if (state) {
        _fullCycle.start();
      } else {
        _fullCycle.stop();
      }
    }

    void run() {
      _fullCycle.run();
    }
};

class Switch : public Controller<bool> {
  private:
    // Does this relay go on when its pin is HIGH or LOW?
    bool _onState;
    uint8_t _pin;

  public:
    Switch(uint8_t pin, bool onState, bool startingState = false)
    : _pin(pin),
      _onState(onState)
    {
      pinMode(_pin, OUTPUT);
      setState(startingState);
    }

    void setState(bool state) {
      digitalWrite(_pin, _onState == state);
    }
};

class Relay : public ThrottledController<bool>, public Controller<bool> {
  public:
    Relay(uint8_t pin, bool onState, unsigned long minCycleTime, bool startingState = false)
    : ThrottledController(
        Switch(pin, onState, startingState),
        minCycleTime
      )
    { }
};

class MotorDriver : public VariableController<float> {
    uint8_t _forwardPin;
    uint8_t _backwardPin;
    uint8_t _pwmPin;
    // Does setting a pin HIGH activate it or deactivate it?
    bool _controlPinActiveState;

  public:
    MotorDriver(uint8_t forwardPin, uint8_t backwardPin, uint8_t pwmPin, bool controlPinActiveState, float startingState = 0.0)
    : _forwardPin(forwardPin),
      _backwardPin(backwardPin),
      _pwmPin(pwmPin),
      _controlPinActiveState(controlPinActiveState)
    {
      pinMode(_forwardPin, OUTPUT);
      pinMode(_backwardPin, OUTPUT);
      if (_pwmPin > 0) {
        pinMode(_pwmPin, OUTPUT);
      }
      setState(startingState);
    }

    void setState(float state) {
      if (_pwmPin > 0) {
        // Normalise -1...0 to 0...1023 and 0...1 to 0...1023 too, to drive PWM pins.
        int pwmValue = round(abs(state) * 1023);
        analogWrite(_pwmPin, pwmValue);
      }

      if (state < 0) {
        digitalWrite(_forwardPin, !_controlPinActiveState);
        digitalWrite(_backwardPin, _controlPinActiveState);
      } else if (state > 0) {
        digitalWrite(_forwardPin, _controlPinActiveState);
        digitalWrite(_backwardPin, !_controlPinActiveState);
      } else {
        digitalWrite(_forwardPin, !_controlPinActiveState);
        digitalWrite(_backwardPin, !_controlPinActiveState);
      }
    }

    Range<float> getRange() {
      const static Range<float> range(-1.0, 1.0);
      return range;
    }
};

// WARNING: This should only be used with an actuator that has a limit switch.
class Cover : public ControllerTranslator<bool, float>, public Controller<bool> {
  public:
    Cover(MotorDriver wrappedController, unsigned long estimatedExcursionTime)
    : ControllerTranslator(
      ThrottledController(wrappedController, estimatedExcursionTime),
      [](bool outerState) {
        return outerState ? 1.0 : -1.0;
      }
    )
    { }
};

class SlowPwmController : public VariableController<float>, public Runnable {
  private:
    Controller<bool> _wrappedController;
    AsyncTimer _timer;
    float _state;
  
  public:
    SlowPwmController(Controller<bool> wrappedController, unsigned long minCycleTime, uint8_t pwmResolution)
      :
        _wrappedController(wrappedController),
        _timer(AsyncTimer(
          minCycleTime,
          0,
          [this, pwmResolution]() {
            // The PWM cycle length is the minimum switching time of the wrapped controller,
            // times the PWM resolution.
            // So if the switching time is 10 ms and the PWM resolution is 10,
            // the cycle length is 10 ms * 10.

            // Normalise to an integer in the PWM resolution's duty cycle 0..pwmResolution.
            uint8_t duty = round(_state * pwmResolution);

            // Count how many steps have passed in this cycle.
            // If the duty is below or equal to that number of steps,
            // turn the wrapped controller.
            _wrappedController.setState(((_timer.getCount() % pwmResolution) <= duty ? true : false));
          }
        ))
    { }

    void setState(float state) {
      _state = state;
      _timer.run();
    }

    Range<float> getRange() {
      const static Range<float> range(0, 1.0);
      return range;
    }

    void run() {
      _timer.run();
    }
};

#endif