#ifndef PAULDAOUST_SENSOR_STREAM_H
#define PAULDAOUST_SENSOR_STREAM_H

#include <optional>
#include <AsyncTimer.h>
#include <Sensor.h>

template <typename T>
class SensorStream : Runnable {
  public:
    struct Item<T> {
      unsigned long timestamp;
      std::optional<T> value;
    };

    virtual std::optional<Item<T>> consume();
};

template <typename T>
class SensorBuffer : SensorStream<T> {
  private:
    std::queue<Item<T>> _buffer;
    AsyncTimer _timer;
  
  public:
    SensorBuffer(Sensor<T> wrappedSensor, unsigned long interval)
    : _timer(AsyncTimer(
        interval,
        0,
        [this, wrappedSensor]() {
          _buffer.push(Item<T>{millis(), wrappedSensor.read()});
        }
      ))
    { }

    std::optional<Item<T>> consume() {
      if (_buffer.size() == 0) {
        return;
      }
      return _buffer.pop();
    }

    void run() {
      _timer.run();
    }
};

#endif