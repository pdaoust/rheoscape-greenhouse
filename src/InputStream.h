#ifndef PAULDAOUST_INPUT_STREAM_H
#define PAULDAOUST_INPUT_STREAM_H

#include <optional>
#include <AsyncTimer.h>
#include <Input.h>

template <typename T>
class InputStream : Runnable {
  public:
    struct Item<T> {
      unsigned long timestamp;
      std::optional<T> value;
    };

    virtual std::optional<Item<T>> consume();
};

template <typename T>
class InputBuffer : InputStream<T> {
  private:
    std::queue<Item<T>> _buffer;
    AsyncTimer _timer;
  
  public:
    InputBuffer(Input<T> wrappedInput, unsigned long interval)
    : _timer(AsyncTimer(
        interval,
        0,
        [this, wrappedInput]() {
          _buffer.push(Item<T>{millis(), wrappedInput.read()});
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