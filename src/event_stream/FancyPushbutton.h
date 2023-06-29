#ifndef RHEOSCAPE_FANCY_PUSHBUTTON_H
#define RHEOSCAPE_FANCY_PUSHBUTTON_H

#include <event_stream/EventStream.h>

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

// Fancy pushbutton, taking in a (debounced) on/off event stream and turning it into a series of events.
// You give a short press time, at or below which a press is considered just a simple press.
// You can also give it a long press time, below which a press is considered a long press.
// If the button is held beyond the long press time, it starts emitting repeat press events at the repeat interval.
//
// Here's the sequence of events for a short press:
//
// 1. button_down
// 2. button_press
// 3. button_up
//
// button_press will be emitted at the same time as button_up, and its timestamp will be the same.
//
// Here's the sequence of events for a long press:
//
// 1. button_down
// 2. button_holdStart
// 3. button_longPress
// 4. button_holdDone
// 5. button_up
//
// Here's the sequence of events for a hold with repeat press events:
//
// 1. button_down
// 2. button_holdStart
// 3..n+3. button_holdRepeatPress ... (n times)
// n+4. button_holdDone
// n+5. button_up
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
      uint16_t longPressTime = 400,
      // Any press longer than a long press will emit press events at this interval.
      uint16_t repeatInterval = 200
    ) :
      Runnable(runner),
      _shortPressTime(shortPressTime),
      _longPressTime(longPressTime),
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
            // We emit an event stamped to the start of the long press, not the current timestamp --
            // even though that time has passed already.
            // Maybe we should set the timestamp to _shortPressTime + 1
            // (which would also necessitate changing the above condition from >= to >)
            // but I think 1 millisecond isn't going to be that surprising.
            unsigned long longPressStartTimestamp = lastEventTimestamp + _shortPressTime;
            _emit(Event<FancyPushbuttonEvent>{
              longPressStartTimestamp,
              button_holdStart
            });
          } else {
            // We don't want to fall through if we haven't hit the end of short press time.
            break;
          }
          // Fall through to repeat press territory in case we've gotten that far..
        case button_holdStart:
          if (lastEventTimestamp + _longPressTime >= now) {
            // We're beyond long press territory, into repeat press territory.
            // Emit the first repeat press event.
            lastRepeatPressTimestamp = lastEventTimestamp + _longPressTime;
            _lastEventEmitted = Event<FancyPushbuttonEvent>{
              lastRepeatPressTimestamp.value(),
              button_holdRepeatPress
            };
            _emit(_lastEventEmitted.value());
          } else {
            break;
          }
          // Fall through to _more_ repeat presses in case we should emit more than one.
        case button_holdRepeatPress:
          if (!lastRepeatPressTimestamp.has_value()) {
            // If we've fallen through from the above case, this will have a value.
            // But if not, we need to assign the timestamp of the last repeat event emitted.
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