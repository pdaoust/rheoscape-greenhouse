#ifndef PAULDAOUST_ASYNC_TIMER_H
#define PAULDAOUST_ASYNC_TIMER_H

#include <Arduino.h>

class Runnable {
  public:
    virtual void run();
};

class AsyncTimer : Runnable {
  protected:
    unsigned long _interval;
    unsigned long _startTime;
    uint16_t _times;
    uint16_t _count;
    bool _state = false;
    const std::function<void()>& _callback;
  
  public:
    // startIn is the number of cycles before the timer triggers for the first time.
    // If 0, it'll trigger immediately.
    // If < 0, it won't trigger until you call reset() again with a zero or positive value.
    void reset(int16_t startIn = 0) {
      _count = 0;
      if (startIn < 0) {
        _startTime = 0;
      } else {
        start(startIn);
      }
    }

    void start(uint16_t startIn = 0, uint16_t times = 0) {
      _startTime = millis() + startIn * _interval;
      if (times > 0) {
        _times = times;
      }
    }

    void stop() {
      _startTime = 0;
    }

    AsyncTimer(
      unsigned long interval,
      uint16_t times,
      const std::function<void()>& callback,
      // > 0 is start in n intervals.
      // 0 is start immediately.
      // -1 is don't start until reset.
      int16_t startIn = 0
    ) :
      _interval(interval),
      _times(times),
      _callback(callback),
      // Gotta set it to something.
      // I could put all the logic in here using ternary,
      // but that'd be ugly so I'm doing it in the body via reset().
      _startTime(0)
    {
      // This will set _startTime to something meaningful.
      reset(startIn);
    }

    void run() {
      // If we've run it enough times, or it hasn't been started yet,
      // don't do anything.
      if ((_times > 0 && _count >= _times) || !_startTime) {
        return;
      }
      if (millis() >= _startTime) {
        _startTime += _interval;
        _callback();
        _count ++;
      }
    }

    unsigned long getInterval() {
      return _interval;
    }

    // Get the number of times this timer has been triggered,
    // not including this time (it'll incremet when the callback is done).
    uint16_t getCount() {
      return _count;
    }
};

class ThrottlingTimer : public AsyncTimer {
  public:
    ThrottlingTimer(unsigned long timeout, const std::function<void()>& callback, uint16_t startIn = 0)
      : AsyncTimer(
        timeout,
        1,
        [callback, this]() {
          callback();
          reset(1);
        },
        startIn
      )
    { }
};

#endif