#ifndef RHEOSCAPE_TIMER_H
#define RHEOSCAPE_TIMER_H

#include <functional>
#include <exception>

#ifdef PLATFORM_ARDUINO
#include <Arduino.h>
#else
#ifdef PLATFORM_DEV_MACHINE
#include <chrono>
#endif
#endif

#include <Runnable.h>

enum TimekeeperSource {
  systemTime,
  simTime
};

class Timekeeper {
  private:
    inline static TimekeeperSource _source = TimekeeperSource::systemTime;
    inline static unsigned long _nowMillisSim;
  
  public:
    static unsigned long nowMillis() {
      switch (Timekeeper::_source) {
        case TimekeeperSource::systemTime:
#ifdef PLATFORM_ARDUINO
          return Timekeeper::nowMillis();
#else
#ifdef PLATFORM_DEV_MACHINE
          return (unsigned long)duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
#endif
        case TimekeeperSource::simTime:
          return Timekeeper::_nowMillisSim;
        default:
          throw std::exception();
      }
    }

    static void setSource(TimekeeperSource source) {
      Timekeeper::_source = source;
      if (source == TimekeeperSource::simTime) {
        Timekeeper::setNowSim(0);
      }
    }

    static void setNowSim(unsigned long millis) {
      if (_source != TimekeeperSource::simTime) {
        std::__throw_invalid_argument("Can't set the time; using system time");
      }
      Timekeeper::_nowMillisSim = millis;
    }

    static void tick(unsigned long millis = 1) {
      if (_source != TimekeeperSource::simTime) {
        std::__throw_invalid_argument("Can't tick; using system time");
      }
      Timekeeper::_nowMillisSim += millis;
    }
};

class Timer : public Runnable {
  protected:
    unsigned long _startTime;
    unsigned long _interval;
    std::function<void(uint16_t)> _callback;
    bool _isComplete;
    bool _isCancelled;
    uint16_t _count;
    std::optional<uint16_t> _times;
    bool _firstRunOnStart;

    void _start() {
      // If the firstRunOnStart flag is set, subtract one interval from the start time.
      _startTime = Timekeeper::nowMillis() - (_firstRunOnStart ? _interval : 0);
      _count = 0;
      _isComplete = false;
      _isCancelled = false;
      run();
    }

  public:
    Timer(unsigned long interval, std::function<void(uint16_t)> callback, std::optional<uint16_t> times = 1, bool firstRunOnStart = false, bool start = true)
    :
      _interval(interval),
      _callback(callback),
      _times(times),
      _firstRunOnStart(firstRunOnStart)
    {
      if (_interval == 0) {
        throw std::invalid_argument("Can't have a zero interval. If you want to run the timer immediately, use the firstRunOnStart flag instead.");
      }
      if (_firstRunOnStart && _times == 1) {
        throw std::invalid_argument("Setting firstRunOnStart to true when there's only one interval makes no sense.");
      }
      if (start) {
        _start();
      }
    }

    Timer(unsigned long interval, std::function<void()> callback, std::optional<uint16_t> times = 1, bool firstRunOnStart = false, bool start = true)
    : Timer(interval, [callback](uint16_t _) { callback(); }, times, firstRunOnStart, start)
    { }

    virtual void run() {
      if (_isComplete || _isCancelled) {
        return;
      }

      unsigned long now = Timekeeper::nowMillis();
      unsigned long elapsed = now - (_startTime + _count * _interval);
      if (elapsed < _interval) {
        return;
      }

      // It may have been a while since it was last run;
      // more than one interval may have passed.
      uint16_t timesToDo = elapsed / _interval;
      if (_times.has_value()) {
          // If we're doing it a limited number of times,
          // and the interval > 1,
          // don't go beyond the remaining number of times.
          // This should work -- integer math throws away the remainder.
        timesToDo = std::min((uint16_t)(_times.value() - _count), timesToDo);
      }

      // Catch up by running it as many times as we need to.
      for (uint16_t i = 0; i < timesToDo; i ++) {
        _callback(_count);
        _count ++;
      }

      if (_times.has_value() && _count == _times.value()) {
        _isComplete = true;
      }
    }

    bool isRunning() {
      run();
      return !_isComplete && !_isCancelled;
    }

    bool isComplete() {
      run();
      return _isComplete;
    }

    bool isCancelled() {
      return _isCancelled;
    }

    uint16_t getCount() {
      run();
      return _count;
    }

    void cancel() {
      if (!_isComplete) {
        _isCancelled = true;
      }
    }

    void restart() {
      _start();
    }
};

// Don't run a function more than every n milliseconds.
// Not a timer, but timer-related.
class Throttle {
  private:
    bool _canRun;
    Timer _timer;
    std::function<void()> _callback;

  public:
    Throttle(unsigned long minDelay, std::function<void()> callback)
    :
      _canRun(true),
      _timer(Timer(minDelay, [this](){ _canRun = true; }, 1)),
      _callback(callback)
    { }

    bool tryRun() {
      _timer.run();
      if (!_canRun) {
        return false;
      }
      _callback();
      _canRun = false;
      _timer.restart();
      return true;
    }

    void clear() {
      _canRun = true;
      _timer.restart();
    }
};

#endif