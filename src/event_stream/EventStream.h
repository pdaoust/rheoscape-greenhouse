#ifndef RHEOSCAPE_EVENT_STREAM_H
#define RHEOSCAPE_EVENT_STREAM_H

#include <optional>
#include <queue>
#include <Arduino.h>

#include <Timer.h>
#include <Runnable.h>

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

// I suppose it's silly to make this; just feel like it makes intentions clear.
template <typename T>
class NullEventStream : public EventStream<T> { };

#endif