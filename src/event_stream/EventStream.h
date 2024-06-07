#ifndef RHEOSCAPE_EVENT_STREAM_H
#define RHEOSCAPE_EVENT_STREAM_H

#include <functional>

#include <helpers/string_format.h>
#include <Runnable.h>
#include <Timer.h>

template <typename T>
struct Event {
  unsigned long timestamp;
  T value;

  Event(unsigned long timestamp, T value)
  : timestamp(timestamp), value(value) { }
};

template <typename T>
class EventStream {
  private:
    std::vector<std::function<void(Event<T>)>> _subscribers;
    std::optional<Event<T>> _lastEvent;

  protected:
    void _emit(Event<T> event) {
      _lastEvent = event;
      for (auto receive : _subscribers) {
        receive(event);
      }
    }

    void _emit(T value) {
      _emit(Event<T>{
        Timekeeper::nowMillis(),
        value
      });
    }

  public:
    void registerSubscriber(std::function<void(Event<T>)> subscriber, bool receiveLastEvent = false) {
      _subscribers.push_back(subscriber);
      if (receiveLastEvent && _lastEvent.has_value()) {
        subscriber(_lastEvent.value());
      }
    }

    std::optional<Event<T>> getLastEvent() const {
      return _lastEvent;
    }
};

// I suppose it's silly to make this; just feel like it makes intentions clear.
template <typename T>
class NullEventStream : public EventStream<T> { };

template <typename T>
class DumbEventStream : public EventStream<T> {
  public:
    DumbEventStream() {}

    void emit(T value) {
      this->_emit(value);
    }
};

#endif