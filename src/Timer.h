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
          return millis();
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
    uint16_t _passedIntervals;
    std::optional<uint16_t> _times;
    bool _firstRunOnStart;
    bool _catchUp;

    void _start() {
      // If the firstRunOnStart flag is set, subtract one interval from the start time.
      _startTime = Timekeeper::nowMillis() - (_firstRunOnStart ? _interval : 0);
      _passedIntervals = 0;
      _isComplete = false;
      _isCancelled = false;
      run();
    }

  public:
    Timer(
      // How often to run the timer.
      unsigned long interval,
      // What to run. The callback can take the count of times it's run so far, not counting the current run.
      std::function<void(uint16_t)> callback,
      // An optional number of times to run, after which it'll stop.
      // times × interval MUST fit into ULONG_MAX.
      // If blank, will keep running forever.
      // NOTE: If blank, the count isn't guaranteed to be accurate.
      std::optional<uint16_t> times = 1,
      // Does the first run happen immediately, or after the first interval has elapsed?
      bool firstRunOnStart = false,
      // Start counting right away, or leave in a non-running state?
      bool start = true,
      // When more than one interval has elapsed, should we try to catch up by running multiple times?
      bool catchUp = false
    )
    :
      _interval(interval),
      _callback(callback),
      _times(times),
      _firstRunOnStart(firstRunOnStart),
      _catchUp(catchUp)
    {
      if (_interval == 0) {
        throw std::invalid_argument("Can't have a zero interval. If you want to run the timer immediately, use the firstRunOnStart flag instead.");
      }
      if (_firstRunOnStart && _times == 1) {
        throw std::invalid_argument("Setting firstRunOnStart to true when there's only one interval makes no sense.");
      }
      if (_times.has_value() && ULONG_MAX / _interval < _times.value()) {
        throw std::invalid_argument("times × interval must be smaller than ULONG_MAX");
      }
      if (start) {
        _start();
      }
    }

    Timer(
      // How often to run the timer.
      unsigned long interval,
      // What to run.
      std::function<void()> callback,
      // An optional number of times to run, after which it'll stop.
      // times × interval MUST fit into ULONG_MAX.
      // If blank, will keep running forever.
      // NOTE: If blank, the count isn't guaranteed to be accurate.
      std::optional<uint16_t> times = 1,
      // Does the first run happen immediately, or after the first interval has elapsed?
      bool firstRunOnStart = false,
      // Start counting right away, or leave in a non-running state?
      bool start = true,
      // When more than one interval has elapsed, should we try to catch up by running multiple times?
      bool catchUp = false
    )
    : Timer(interval, [callback](uint16_t _) { callback(); }, times, firstRunOnStart, start, catchUp)
    { }

    virtual void run() {
      if (_isComplete || _isCancelled) {
        return;
      }

      unsigned long now = Timekeeper::nowMillis();
      unsigned long elapsed = now - _startTime;
      if (elapsed < _interval) {
        return;
      }

      // It may have been a while since it was last run;
      // more than one interval may have passed.
      uint16_t elapsedIntervals = elapsed / _interval;

      if (_catchUp) {
        uint16_t timesToDo = elapsedIntervals;
        if (_times.has_value() && timesToDo > 1) {
            // If we're doing it a limited number of times,
            // don't go beyond the remaining number of times.
          timesToDo = std::min((uint16_t)(_times.value() - _passedIntervals), timesToDo);
        }

        // Catch up by running it as many times as we need to.
        for (uint16_t i = 0; i < timesToDo; i ++) {
          _callback(_passedIntervals + i);
        }
      } else {
        // If we're not supposed to catch up, just jump to the elapsed intervals.
        _callback(_passedIntervals + elapsedIntervals);
      }

      _passedIntervals += elapsedIntervals;

      if (_times.has_value() && _passedIntervals == _times.value()) {
        _isComplete = true;
      }

      // Get ready to run the next time.
      _startTime += elapsedIntervals * _interval;
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
      return _passedIntervals;
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