#ifndef RHEOSCAPE_TIME_PROCESSES_H
#define RHEOSCAPE_TIME_PROCESSES_H

#include <input/Input.h>

// Take 'continuous' values and 'snap' them to a time interval.
template <typename T>
class TimeQuantisingProcess : public Input<T> {
  private:
    Input<T> _wrappedInput;
    RepeatTimer _timer;
    std::optional<T> _lastReadValue;
  
  public:
    TimeQuantisingProcess(Input<T> wrappedInput, unsigned long interval)
    :
      _wrappedInput(wrappedInput),
      _timer(RepeatTimer(
        millis(),
        interval,
        [this]() {
          _lastReadValue = _wrappedInput.read();
        }
      ))
    { }

    T read() {
      _timer.tick();
      return _lastReadValue;
    }
};

// Don't let a value change more frequently than every n milliseconds.
// Kinda like TimeQuantisingProcess, except it doesn't 'snap' to the interval.
template <typename T>
class ThrottlingProcess : public Input<T> {
  private:
    Input<T> _wrappedInput;
    unsigned long _minDelay;
    T _lastReadValue;
    Timer _timer;

  public:
    ThrottlingProcess(Input<T> wrappedInput, unsigned long minDelay)
    :
      _wrappedInput(wrappedInput),
      _minDelay(minDelay),
      _timer(Timer(0, [](){}))
    { }

    T read() {
      // Maybe I coulda figured out how to use my own Throttle
      // (which I suspect I built for this purpose) for this process,
      // but I couldn't figure out how to make it value-aware --
      // that is, to understand that only a changed value resets the throttle.
      if (_timer.isRunning()) {
        return _lastReadValue;
      }

      T value = _wrappedInput.read();
      if (value != _lastReadValue) {
        _timer.restart(millis() + _minDelay);
        _lastReadValue = value;
      }
      return _lastReadValue;
    }
};

// Convert a boolean input to another boolean input,
// where true is converted to a true/false pulse.
class BlinkingProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
    RepeatTimer _fullCycleTimer;
    Timer _offTimer;
    unsigned long _onTime;
  
  public:
    BlinkingProcess(Input<bool> wrappedInput, unsigned long onTime, unsigned long offTime)
    :
      _wrappedInput(wrappedInput),
      _onTime(onTime),
      _fullCycleTimer(RepeatTimer(
        0,
        onTime + offTime,
        [this]() {
          _offTimer.restart(millis() + _onTime);
        }
      )),
      _offTimer(Timer(
        0,
        [](){}
      ))
    { }

    bool read() {
      bool innerValue = _wrappedInput.read();
      if (innerValue) {
        if (!_fullCycleTimer.isRunning()) {
          _fullCycleTimer.restart(millis());
        }
        return _offTimer.isRunning();
      } else {
        _fullCycleTimer.cancel();
        _offTimer.cancel();
      }
    }
};

// Convert a 0..1 float to a boolean suitable for using in slow PWM outputs.
class SlowPwmProcess : public Input<bool> {
  private:
    RepeatTimer _timer;
    bool _currentValue;
  
  public:
    SlowPwmProcess(
      Input<float> wrappedInput,
      // The smallest discrete value in a cycle.
      unsigned long interval,
      // The number of intervals in a cycle. interval * resolution = cycle length.
      uint8_t resolution
    ) :
      _timer(RepeatTimer(
        millis(),
        interval,
        [this, &wrappedInput, resolution]() {
          float value = wrappedInput.read();
          unsigned long ticksPassed = _timer.getCount() % resolution;
          float dutyCycle = ticksPassed / resolution;
          _currentValue = ticksPassed <= value;
        }
      ))
    { }

    bool read() {
      _timer.tick();
      return _currentValue;
    }
};

// Add a delay into a input's reading,
// where it takes a given amount of time to reach the new reading.
// For example, for a hysteresiser that can only move 1 step per second,
// if the underlying input read 10 a second ago and now reads 20,
// it will actually read 11 now, 12 next second, and so forth
// until it finally reads 20 in 10 seconds.
// Of course, if the input goes back down to 10 the next second,
// it'll head towards that value.
template <typename T>
class HysteresisProcess : public Input<T> {
  private:
    Input<T> _wrappedInput;
    unsigned long _interval;
    T _stepsPerInterval;
    T _lastValue;
    unsigned long _lastRun;

  public:
    HysteresisProcess(Input<T> wrappedInput, unsigned long interval, T stepsPerInterval)
    :
      _wrappedInput(wrappedInput),
      _interval(interval),
      _stepsPerInterval(stepsPerInterval)
    { }
  
    T read() {
      T newValue = _wrappedInput.read();
      unsigned long now = millis();
      if (_lastRun) {
        _lastValue = _lastValue + (float)(now - _lastRun) / _interval * _stepsPerInterval;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
    }
};

// Smooth a input reading over a moving average time interval in milliseconds,
// using the exponential moving average or single-pole IIR method.
template <typename T>
class ExponentialMovingAverageProcess : public Input<T> {
  private:
    Input<T> _wrappedInput;
    unsigned long _averageOver;
    T _lastValue;
    unsigned long _lastRun;
  
  public:
    ExponentialMovingAverageProcess(Input<T> wrappedInput, unsigned long averageOver)
    :
      _wrappedInput(wrappedInput),
      _averageOver(averageOver)
    { }

    T read() {
      T newValue = _wrappedInput.read();
      unsigned long now = millis();
      if (_lastRun) {
        float intervalsSinceLastRun = (float)(now - _lastRun) / _averageOver;
        _lastValue -= _lastValue / intervalsSinceLastRun;
        _lastValue += newValue / intervalsSinceLastRun;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
    }
};

// When the input goes true, emit true for a given number of milliseconds,
// then revert to false regardless of whether the input is true or false.
class TimedLatchProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
    Timer _timer;
    unsigned long _timeout;
    bool _previousValue;
  
  public:
    TimedLatchProcess(Input<bool> wrappedInput, unsigned long timeout)
    :
      _wrappedInput(wrappedInput),
      _timeout(timeout),
      _timer(Timer(
        0,
        []() {
          // We don't actually need a value;
          // we're just using this timer to test whether it's running.
        }
      ))
    { }

    bool read() {
      _timer.tick();
      if (_timer.isRunning()) {
        // Within the timer window.
        return true;
      }
      std::optional<bool> inputValue = _wrappedInput.read();
      if (!_previousValue && inputValue) {
        // 'rising edge'
        _timer.restart(millis() + _timeout);
        return true;
      } else {
        // Neither rising edge nor within the timer window.
        return false;
      }
    }
};

#endif