#ifndef RHEOSCAPE_TIMER_H
#define RHEOSCAPE_TIMER_H

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

      if (millis() >= _when && !_isDone) {
        _callback();
        _isDone = true;
      }
    }

    bool isRunning() {
      return _when && !_isDone;
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
class RepeatTimer : private Timer {
  private:
    uint16_t _count;
    bool _isRunning = false;

  public:
    RepeatTimer(
      // When to run the first time.
      unsigned long startAt,
      // How often to run.
      // Note that the tick() function should be called more often than this interval,
      // or it will miss some calls.
      unsigned long interval,
      // The function to run.
      const std::function<void()>& callback,
      // How many times to run (zero for infinite).
      uint16_t times = 0
    ) :
      Timer(startAt, [this, startAt, interval, times, callback]() {
        _isRunning = true;
        if (!times || _count < times) {
          callback();
          // This is one of the lines where the count will be off if tick() isn't called often enough.
          _count ++;
          if (_count == times) {
            _isRunning = false;
            return;
          }
          // This is another.
          Timer::restart(startAt + _count * interval);
        }
     })
    { }

    void tick() { Timer::tick(); }

    bool isRunning() {
      return Timer::isRunning() && _isRunning;
    }

    bool isDone() { return Timer::isDone(); }

    void restart(unsigned long when) { Timer::restart(when); }

    void cancel() { Timer::cancel(); }

    bool getCount() {
      return _count;
    }
};

// Don't run a function more than every n milliseconds.
// Not a timer, but timer-related.
class Throttle {
  private:
    unsigned long _minDelay;
    Timer _timer;
    const std::function<void()>& _callback;

  public:
    Throttle(unsigned long minDelay, const std::function<void()>& callback)
    :
      _minDelay(minDelay),
      _timer(Timer(0, [](){})),
      _callback(callback)
    { }

    bool tryRun() {
      if (_timer.isRunning()) {
        return false;
      }
      _callback();
      _timer.restart(_minDelay);
      return true;
    }
};

#endif