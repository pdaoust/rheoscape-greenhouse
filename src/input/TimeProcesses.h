#ifndef RHEOSCAPE_TIME_PROCESSES_H
#define RHEOSCAPE_TIME_PROCESSES_H

#include <math.h>

//#include <helpers/string_format.h>
#include <input/Input.h>
#include <Timer.h>

// Take 'continuous' values and 'snap' them to a time interval.
template <typename T>
class TimeQuantisingProcess : public Input<T> {
  private:
    Input<T>* _wrappedInput;
    Timer _timer;
    T _lastReadValue;
  
  public:
    TimeQuantisingProcess(Input<T>* wrappedInput, unsigned long interval)
    :
      _wrappedInput(wrappedInput),
      _timer(Timer(
        interval,
        [this]() {
          _lastReadValue = _wrappedInput->read();
        },
        std::nullopt,
        true
      ))
    { }

    virtual T read() {
      _timer.run();
      return _lastReadValue;
    }
};

// Don't let a value change more frequently than every n milliseconds.
// Kinda like TimeQuantisingProcess, except it doesn't 'snap' to the interval.
template <typename T>
class ThrottlingProcess : public Input<T> {
  private:
    Input<T>* _wrappedInput;
    unsigned long _minDelay;
    T _lastReadValue;
    bool _canReadAgain;
    Throttle _throttle;

  public:
    ThrottlingProcess(Input<T>* wrappedInput, unsigned long minDelay)
    :
      _wrappedInput(wrappedInput),
      _canReadAgain(true),
      _throttle(Throttle(minDelay, [this](){ _canReadAgain = true; }))
    { }

    virtual T read() {
      _throttle.tryRun();
      if (!_canReadAgain) {
        return _lastReadValue;
      }

      T value = _wrappedInput->read();
      if (value != _lastReadValue) {
        _canReadAgain = false;
        _lastReadValue = value;
      } else {
        _throttle.clear();
      }
      return _lastReadValue;
    }
};

// Convert a boolean input to another boolean input,
// where true is converted to a true/false pulse.
class BlinkingProcess : public Input<bool> {
  private:
    Input<bool>* _wrappedInput;
    Timer _offTimer;
    Timer _fullCycleTimer;
    bool _state;
  
  public:
    BlinkingProcess(Input<bool>* wrappedInput, unsigned long onTime, unsigned long offTime)
    :
      _wrappedInput(wrappedInput),
      _offTimer(Timer(
        offTime,
        [this](){
          _state = false;
        },
        1,
        false,
        true
      )),
      _fullCycleTimer(Timer(
        onTime + offTime,
        [this]() {
          _state = true;
          _offTimer.restart();
        },
        std::nullopt,
        true,
        false
      ))
    { }

    virtual bool read() {
      bool innerValue = _wrappedInput->read();
      if (innerValue) {
        if (!_fullCycleTimer.isRunning()) {
          _fullCycleTimer.restart();
        } else {
          _fullCycleTimer.run();
        }
        _offTimer.run();
      } else {
        _fullCycleTimer.cancel();
        _offTimer.cancel();
        _state = false;
      }
      return _state;
    }
};

// Convert a 0..1 float to a boolean suitable for using in slow PWM outputs.
class SlowPwmProcess : public Input<bool> {
  private:
    Timer _timer;
    bool _currentValue;
    uint16_t _rollovers;
    uint16_t _lastCounter;
  
  public:
    SlowPwmProcess(
      Input<float>* wrappedInput,
      // The length of a cycle.
      unsigned long interval,
      // The number of steps in a cycle. interval / resolution = step duration.
      // Make sure this interval is divisible by resolution, cuz we aren't doing float math on timespans!
      uint8_t resolution
    ) :
      _rollovers(0),
      _lastCounter(0),
      _timer(Timer(
        interval / resolution,
        [this, &wrappedInput, resolution](uint16_t count) {
          // The resolution of the counter isn't enough to reliably cross rollovers.
          // This keeps an internal rollover monitor and allows accurate rollover crossing.
          if (_lastCounter > count) {
            _rollovers ++;
          }
          _lastCounter = count;

          float value = wrappedInput->read();
          uint16_t stepsPassedInInterval = (count + UINT16_MAX % resolution * _rollovers + _rollovers) % resolution;
          float stepsPassedAsFraction = (float)stepsPassedInInterval / (float)resolution;
          _currentValue = stepsPassedAsFraction < value;
        },
        std::nullopt
      ))
    {
      if (interval % resolution != 0) {
        throw std::invalid_argument("Interval must be a multiple of resolution.");
      }
    }

    bool read() {
      _timer.run();
      return _currentValue;
    }
};

// Add a delay into an input's reading,
// where it takes a given amount of time to reach the new reading.
// For example, for a hysteresiser that can only move 1 step per second,
// if the underlying input read 10 a second ago and now reads 20,
// it will actually read 11 now, 12 next second, and so forth
// until it finally reads 20 in 10 seconds.
// Of course, if the input goes back down to 10 the next second,
// it'll head back towards that value.
// NOTE: Because of the limitations of template types,
// you must use a signed type for T.
template <typename T>
class HysteresisProcess : public Input<T> {
  private:
    Input<T>* _wrappedInput;
    unsigned long _interval;
    T _stepsPerInterval;
    std::optional<std::tuple<T, unsigned long>> _lastResult;

  public:
    HysteresisProcess(Input<T>* wrappedInput, unsigned long interval, T stepsPerInterval)
    :
      _wrappedInput(wrappedInput),
      _interval(interval),
      _stepsPerInterval(stepsPerInterval)
    { }
  
    virtual T read() {
      T newValue = _wrappedInput->read();
      unsigned long now = Timekeeper::nowMillis();
      if (_lastResult.has_value()) {
        T lastValue = std::get<0>(_lastResult.value());
        unsigned long lastTimestamp = std::get<1>(_lastResult.value());
        // Calculate the amount of allowed change for the period.
        T maxAmount = (now - lastTimestamp) / _interval * _stepsPerInterval;
        T amount = newValue > lastValue
          // The amount of change may be less than the calculated allowed change for the period.
          ? std::min(maxAmount, newValue - lastValue)
          : newValue < lastValue
            ? std::max(-maxAmount, newValue - lastValue)
            : 0;
        T interpolated = lastValue + amount;
        _lastResult = std::tuple<T, unsigned long>(interpolated, now);
      } else {
        _lastResult = std::tuple<T, unsigned long>(newValue, now);
      }
      return std::get<0>(_lastResult.value());
    }
};

// Smooth an input reading over a moving average time interval in milliseconds,
// using the exponential moving average or single-pole IIR method.
template <typename T>
class ExponentialMovingAverageProcess : public Input<T> {
  private:
    Input<T>* _wrappedInput;
    unsigned long _timeConstant;
    std::optional<std::tuple<T, unsigned long>> _lastResult;
    //std::string _message;
  
  public:
    ExponentialMovingAverageProcess(
      Input<T>* wrappedInput,
      /// If you think of this process as a low-pass filter,
      /// the time constant is the period of the cutoff frequency.
      /// Any movements slower than this will get integrated;
      /// any movements faster than this will get filtered out.
      unsigned long timeConstant
    )
    :
      _wrappedInput(wrappedInput),
      _timeConstant(timeConstant)
    { }

    virtual T read() {
      T newValue = _wrappedInput->read();
      unsigned long now = Timekeeper::nowMillis();
      if (_lastResult.has_value()) {
        // Taken from https://stackoverflow.com/a/1027808
        T lastValue = std::get<0>(_lastResult.value());
        unsigned long lastTimestamp = std::get<1>(_lastResult.value());
        unsigned long timeDelta = now - lastTimestamp;
        // Alpha AKA decay for the given time delta.
        float alpha = 1.0f - powf((float)M_E, -(float)timeDelta / (float)_timeConstant);
        T integrated = lastValue + alpha * (newValue - lastValue);
        _lastResult = std::tuple<T, unsigned long>(integrated, now);
        //_message = string_format("lastValue is %.3f, lastTimestamp is %d, timeDelta is %d, alpha is %.3f, integrated is %.3f (time constant is %d)", lastValue, lastTimestamp, timeDelta, alpha, integrated, _timeConstant);
      } else {
        _lastResult = std::tuple<T, unsigned long>(newValue, now);
        //_message = "first run";
      }

      return std::get<0>(_lastResult.value());
    }

    //std::string getMessage() { return _message; }
};

// When the input goes true, emit true for a given number of milliseconds,
// then revert to false regardless of whether the input is true or false.
class TimedLatchProcess : public Input<bool> {
  private:
    Input<bool>* _wrappedInput;
    Timer _timer;
    bool _previousValue;
  
  public:
    TimedLatchProcess(Input<bool>* wrappedInput, unsigned long timeout)
    :
      _wrappedInput(wrappedInput),
      _timer(Timer(
        timeout,
        []() {
          // We don't actually need a value;
          // we're just using this timer to test whether it's running.
        },
        1,
        false,
        false
      ))
    { }

    virtual bool read() {
      _timer.run();
      if (_timer.isRunning()) {
        // Within the timer window.
        return true;
      }
      bool inputValue = _wrappedInput->read();
      if (!_previousValue && inputValue) {
        // 'rising edge'
        _timer.restart();
        _previousValue = inputValue;
        return true;
      } else {
        // Neither rising edge nor within the timer window.
        _previousValue = inputValue;
        return false;
      }
    }
};

#endif