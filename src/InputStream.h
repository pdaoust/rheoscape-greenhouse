#ifndef PAULDAOUST_INPUT_STREAM_H
#define PAULDAOUST_INPUT_STREAM_H

#include <optional>
#include <queue>
#include <Timer.h>
#include <Input.h>
#include <Runnable.h>

template <typename T>
struct Event {
  unsigned long timestamp;
  std::optional<T> value;
};

template <typename T>
class InputStream {
  private:
    std::queue<Event<T>> _buffer;

  protected:
    void _produce(std::optional<T> value, unsigned long timestamp = 0) {
      _buffer.push(Event<T>{timestamp ? timestamp : millis(), value});
    }

  public:
    std::optional<Event<T>> consume() {
      if (_buffer.size() == 0) {
        return;
      }
      return _buffer.pop();
    }
};

// Read and buffer the input value every n milliseconds.
template <typename T>
class InputBuffer : public InputStream<T>, Runnable {
  private:
    RepeatTimer _timer;
  
  public:
    InputBuffer(Input<T> wrappedInput, unsigned long interval)
    : _timer(RepeatTimer(
        millis(),
        interval,
        [this, wrappedInput]() {
          _produce(wrappedInput.read());
        }
      ))
    { }

    void run() {
      _timer.tick();
    }
};

enum FancyPushbuttonEvent {
  down,
  up,
  press,
  // The button has been held for more than a short press period.
  // We're now into long press territory.
  holdStart,
  holdDone
};

class FancyPushbutton : public InputStream<FancyPushbuttonEvent>, Runnable {
  private:
    Input<bool> _wrappedInput;
    uint16_t _shortPressTime;
    uint16_t _longPressTime;
    uint16_t _repeatPeriod;
    bool _inputState;
    bool _longPressStarted;
    unsigned long _lastPressStart;
    unsigned long _lastRepeat;

  public:
    FancyPushbutton(
      Input<bool> wrappedInput,
      // If a press is as long as this or shorter, it'll be considered a short press.
      uint16_t shortPressTime = 200,
      // If a press is longer than a short press, but as long as this or shorter, it'll be considered a long press.
      uint16_t holdTime = 400,
      // Any press longer than a long press will emit press events with this period.
      uint16_t repeatPeriod = 200
    ) :
      _wrappedInput(wrappedInput),
      _shortPressTime(shortPressTime),
      _longPressTime(holdTime),
      _repeatPeriod(repeatPeriod)
    { }
  
    void run() {
      std::optional<bool> state = _wrappedInput.read();
      if (!state.has_value()) {
        state = false;
      }

      unsigned long now = millis();
      if (!_inputState && state) {
        // Button pressed.
        _inputState = true;
        _lastPressStart = now;
        _lastRepeat = 0;
        _longPressStarted = false;
        _produce(down);
      } else if (_inputState && !state) {
        // Button released.
        if (now - _lastPressStart <= _shortPressTime) {
          // And it'd been pressed for a short time.
          _produce(press);
        } else if (_longPressStarted) {
          // And it'd been pressed for a long time but hadn't repeated yet.
          _produce(holdDone);
        }
        // If neither of the two previous conditions are met,
        // it'd been pressed for longer than the long press time, and there were some repeats in there.
        // This isn't considered a press, so don't emit any more events other than button up.
        _produce(up);
      } else if (_inputState && state) {
        if (now - _lastPressStart > _shortPressTime && now - _lastPressStart <= _longPressTime && !_longPressStarted) {
          // We've begun long press time.
          _produce(holdStart);
        } else if (now - _lastPressStart > _longPressTime + _repeatPeriod) {
          // We're heading into repeats.
          if (!_lastRepeat) {
            // This is the first time around.
            // Set the last repeat to the end of the long press time.
            // This wasn't actually the first repeat, but it makes the following math work.
            _lastRepeat = _lastPressStart + _longPressTime;
          }

          for (; _lastRepeat <= now; _lastRepeat += _repeatPeriod) {
            _produce(press, _lastRepeat);
          }
        }
      }
    }
};

#endif