#ifndef PAULDAOUST_EVENT_STREAM_H
#define PAULDAOUST_EVENT_STREAM_H

#include <optional>
#include <queue>
#include <Arduino.h>
#include <Timer.h>
#include <Input.h>
#include <Runnable.h>

template <typename T>
struct Event {
  unsigned long timestamp;
  T value;
};

template <typename T>
class EventStream {
  private:
    std::vector<std::function<void(Event<T>)>> _subscribers;

  protected:
    void _emit(Event<T> event) {
      for (auto receive : _subscribers) {
        receive(event);
      }
    }

    void _emit(T value) {
      _emit(Event<T>{
        millis(),
        value
      });
    }

  public:
    void registerSubscriber(std::function<void(Event<T>)> subscriber) {
      _subscribers.push_back(subscriber);
    }
};

template <typename T>
class InputToEventStream : public EventStream<std::optional<T>>, public Runnable {
  private:
    Input<T> _wrappedInput;
    std::optional<T> _lastSeenValue;
  
  public:
    InputToEventStream(Input<T> wrappedInput)
    : _wrappedInput(wrappedInput)
    { }

    void run() {
      std::optional<T> value = _wrappedInput.read();
      if (value != _lastSeenValue) {
        _lastSeenValue = value;
        _emit(value);
      }
    }
};

template <typename T>
class EventStreamFilter : public EventStream<T> {
  private:
    const std::function<bool(T)>& _filter;

  public:
    void receiveEvent(Event<T> event) {
      if (_filter(event.value)) {
        _emit(event);
      }
    }

    EventStreamFilter(EventStream<T> wrappedEventStream, const std::function<bool(T)>& filter)
    : _filter(filter)
    {
      wrappedEventStream.registerSubscriber([this](Event<T> v) { this->receiveEvent(v); });
    }
};

template <typename TIn, typename TOut>
class EventStreamTranslator : public EventStream<TOut> {
  private:
    const std::function<TOut(TIn)>& _translator;

  public:
    EventStreamTranslator(EventStream<TIn> wrappedEventStream, const std::function<TOut(TIn)>& translator)
    : _translator(translator)
    {
      wrappedEventStream.registerSubscriber([this](Event<TIn> v) { this->receiveEvent(v); });
    }

    void receiveEvent(Event<TIn> event) {
      TOut translated = _translator(event.value);
      _emit(Event<TOut>{
        event.timestamp,
        translated
      });
    }
};

template <typename T>
class EventStreamNotEmpty : public EventStreamTranslator<std::optional<T>, T> {
  public:
    EventStreamNotEmpty(EventStream<std::optional<T>> wrappedEventStream)
    :
      EventStreamTranslator<std::optional<T>, T>(
        EventStreamFilter<std::optional<T>>(
          wrappedEventStream,
          [](std::optional<T> value) {
            return value.has_value();
          }
        ),
        [](std::optional<T> value) {
          return value.value();
        }
      )
    { }
};

template <typename T>
class InputToEventStreamNotEmpty : public EventStreamNotEmpty<T>, public Runnable {
  private:
    Input<T> _wrappedInput;

  public:
    InputToEventStreamNotEmpty(Input<T> wrappedInput, Runner runner)
    :
      Runnable(runner),
      EventStreamNotEmpty<T>(
        InputToEventStream<T>(wrappedInput)
      )
    { }

    void run() {
      _wrappedInput.run();
    }
};

template <typename T>
class EventStreamDebouncer : public EventStream<T>, public Runnable {
  private:
    unsigned long _delay;
    std::optional<Event<T>> _previousEvent;

  public:
    EventStreamDebouncer(EventStream<T> wrappedEventStream, unsigned long delay, Runner runner)
    :
      Runnable(runner),
      _delay(delay),
      _previousEvent(std::nullopt)
    {
      wrappedEventStream.registerSubscriber([this](Event<T> v) { this->receiveEvent(v); });
    }

    void receiveEvent(Event<T> event) {
      // Here we only check for the age of the last delta event.
      // If it's older than the delay, and it hasn't changed, leave it alone --
      // it's settled, and it'll become the source for the next event.
      // If it's younger than the delay, leave it alone regardless.
      // If it's older than the delay and it has changed, register it as the new delta event.
      // And if we've never received an event, of course register it as the new delta event.
      if (!_previousEvent.has_value() || (_previousEvent.value().timestamp + _delay <= event.timestamp && _previousEvent.value().value != event.value)) {
        _previousEvent = event;
      }
    }

    void run() {
      if (_previousEvent.has_value() && _previousEvent.value().timestamp + _delay <= millis()) {
        // The only way the previous delta event can be old enough is if it's settled down.
        // We emit the event with its original timestamp (which will now be at least as old as the debounce delay).
        _emit(_previousEvent.value());
      }
    }
};

enum FancyPushbuttonEvent {
  button_down,
  button_up,
  button_press,
  button_longPress,
  // The button has been held for more than a short press period.
  // We're now into long press territory.
  button_holdStart,
  button_holdRepeatPress,
  button_holdDone
};

class FancyPushbutton : public EventStream<FancyPushbuttonEvent>, public Runnable {
  private:
    const uint16_t _shortPressTime;
    const uint16_t _longPressTime;
    const uint16_t _repeatInterval;
    std::optional<Event<bool>> _lastEventReceived;
    std::optional<Event<FancyPushbuttonEvent>> _lastEventEmitted;

  public:
    FancyPushbutton(
      EventStream<bool> wrappedEventStream,
      Runner runner,
      // If a press is as long as this or shorter, it'll be considered a short press.
      uint16_t shortPressTime = 200,
      // If a press is longer than a short press, but as long as this or shorter, it'll be considered a long press.
      uint16_t holdTime = 400,
      // Any press longer than a long press will emit press events at this interval.
      uint16_t repeatInterval = 200
    ) :
      Runnable(runner),
      _shortPressTime(shortPressTime),
      _longPressTime(holdTime),
      _repeatInterval(repeatInterval)
    {
      wrappedEventStream.registerSubscriber([this](Event<bool> v) { this->receiveEvent(v); });
    }
  
    void receiveEvent(Event<bool> event) {
      if (event.value && (!_lastEventReceived.has_value() || !_lastEventReceived.value().value)) {
        // Last event seen was false and the new one is true; transition to down.
        _lastEventEmitted = Event<FancyPushbuttonEvent>{
          millis(),
          button_down
        };
        _emit(_lastEventEmitted.value());
      } else if (!event.value && _lastEventReceived.has_value() && _lastEventReceived.value().value) {
        // Last event seen was true and new one is false; transitioning to up.
        // First, call run() just in case it hasn't been called at a regular enough interval.
        // This will emit any missed events, in an idempotent way
        // (that is, it will only emit events that haven't already been emitted).
        run();
        // If the last event received was true, we know that something has already been emitted;
        // therefore, skip the `has_value()` check.
        FancyPushbuttonEvent lastPushbuttonEventEmitted = _lastEventEmitted.value().value;
        unsigned long now = millis();
        switch (lastPushbuttonEventEmitted) {
          case button_down:
            _emit(Event<FancyPushbuttonEvent>{now, button_press});
            _emit(Event<FancyPushbuttonEvent>{now, button_up});
            break;
          case button_holdStart:
            _emit(Event<FancyPushbuttonEvent>{now, button_longPress});
            _emit(Event<FancyPushbuttonEvent>{now, button_holdDone});
            _emit(Event<FancyPushbuttonEvent>{now, button_up});
            break;
          case button_holdRepeatPress:
            _emit(Event<FancyPushbuttonEvent>{now, button_holdDone});
            _emit(Event<FancyPushbuttonEvent>{now, button_up});
        }
        // Rather than keep track of all those events we just emitted, just reset it.
        // We only needed to keep track of it so we knew what kinds of fancy event(s) to emit during `run()`.
        _lastEventEmitted = std::nullopt;
      }
    }

    void run() {
      if (!_lastEventEmitted.has_value()) {
        // We're not tracking a down; nothing to do.
        return;
      }

      unsigned long lastEventTimestamp = _lastEventEmitted.value().timestamp;
      FancyPushbuttonEvent lastEvent = _lastEventEmitted.value().value;
      unsigned long now = millis();
      std::optional<unsigned long> lastRepeatPressTimestamp;
      switch (lastEvent) {
        case button_down:
          if (lastEventTimestamp + _shortPressTime >= now) {
            // Passed the short press time; we're now into long press territory.
            // We emit an event for the start of the long press, not now.
            // Maybe we should set the timestamp to _shortPressTime + 1
            // (which would also necessitate changing the above condition from >= to >)
            // but I think 1 millisecond isn't going to be that surprising.
            unsigned long longPressStartTimestamp = lastEventTimestamp + _shortPressTime;
            _emit(Event<FancyPushbuttonEvent>{
              longPressStartTimestamp,
              button_holdStart
            });
            // We also emit the first repeat press event.
            lastRepeatPressTimestamp = longPressStartTimestamp;
            _lastEventEmitted = Event<FancyPushbuttonEvent>{
              longPressStartTimestamp,
              button_holdRepeatPress
            };
            _emit(_lastEventEmitted.value());
          } else {
            // We don't want to fall through if we haven't hit the end of short press time.
            break;
          }
          // Fall through to repeat press territory to calculate how many more repeat presses to do beyond the first one.
        case button_holdRepeatPress:
          if (!lastRepeatPressTimestamp.has_value()) {
            lastRepeatPressTimestamp = lastEventTimestamp;
          }
          for (
            // Skip the event already emitted.
            unsigned long i = lastRepeatPressTimestamp.value() + _repeatInterval;
            // Include the one we need to emit now.
            i <= now;
            i += _repeatInterval
          ) {
            _lastEventEmitted = Event<FancyPushbuttonEvent>{i, button_holdRepeatPress};
            _emit(_lastEventEmitted.value());
          }
      }
    }
};

#endif