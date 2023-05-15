#ifndef PAULDAOUST_TIMER_H
#define PAULDAOUST_TIMER_H

#include <Arduino.h>

class Timer {
  protected:
    unsigned long _when;
    const std::function<void()>& _callback;
    bool _isDone;
  
  public:
    Timer(unsigned long when, const std::function<void()>& callback)
    :
      _when(when),
      _callback(callback)
    { }

    void tick() {
      if (!_when) {
        return;
      }

      if (millis() >= _when && !isDone()) {
        _callback();
        _isDone = true;
      }
    }

    bool isDone() {
      return _isDone;
    }

    void restart(unsigned long when) {
      _when = when;
      _isDone = false;
    }

    void cancel() {
      _when = 0;
    }
};

// Call a callback over and over again, and/or up to a certain number of times.
class RepeatTimer : public Timer {
  private:
    unsigned long _startAt;
    unsigned long _interval;
    uint16_t _times;
    uint16_t _count;
    bool _isRunning = false;
    bool _isDone = false;
    const std::function<void()>& _innerCallback;

    void _tick() {
      _isRunning = true;
      if (!_times || _count < _times) {
        _innerCallback();
        // This is one of the lines where the count will be off if tick() isn't called often enough.
        _count ++;
        if (_count < _times) {
          // This is another.
          Timer::restart(_startAt + _count * _interval);
        } else {
          _isRunning = false;
          _isDone = true;
        }
      }
    }

  public:
    RepeatTimer(
      // When to run the first time.
      unsigned long startAt,
      // How often to run.
      // Note that the tick() function should be called more often than this interval,
      // or it will miss some calls.
      unsigned long interval,
      // How many times to run (zero for infinite).
      uint16_t times,
      // The function to run.
      const std::function<void()>& callback
    ) :
      Timer(startAt, [this]() {
        _tick();
      }),
      _interval(interval),
      _times(times),
      _innerCallback(callback)
    { }

    bool isRunning() {
      return _isRunning;
    }

    bool getCount() {
      return _count;
    }
};

#endif