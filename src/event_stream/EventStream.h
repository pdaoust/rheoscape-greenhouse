#ifndef RHEOSCAPE_EVENT_STREAM_H
#define RHEOSCAPE_EVENT_STREAM_H

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

  protected:
    void _emit(Event<T> event) {
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
    void registerSubscriber(std::function<void(Event<T>)> subscriber) {
      _subscribers.push_back(subscriber);
    }
};

// I suppose it's silly to make this; just feel like it makes intentions clear.
template <typename T>
class NullEventStream : public EventStream<T> { };

template <typename T>
class BeaconEventStream : public EventStream<T>, public Runnable {
  private:
    Timer _timer;
  
  public:
    BeaconEventStream(Input<T>* input, unsigned long interval)
    : 
      Runnable(),
      _timer(Timer(
        interval,
        [input, this]() {
          this->_emit(input->read());
        },
        std::nullopt,
        true
      ))
    { }

    BeaconEventStream(T value, unsigned long interval)
    : BeaconEventStream(new ConstantInput(value), interval)
    { }

    virtual void run() {
      _timer.run();
    }
};

template <typename T>
class DumbEventStream : public EventStream<T> {
  public:
    DumbEventStream() {}

    void emit(T value) {
      this->_emit(value);
    }
};

#endif