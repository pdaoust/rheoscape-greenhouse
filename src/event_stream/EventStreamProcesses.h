#ifndef RHEOSCAPE_EVENT_STREAM_PROCESSES_H
#define RHEOSCAPE_EVENT_STREAM_PROCESSES_H

#include <functional>

#include <Runnable.h>
#include <input/Input.h>
#include <event_stream/EventStream.h>

template <typename T>
class InputToEventStream : public EventStream<T>, public Runnable {
  private:
    Input<T>* _wrappedInput;
    std::optional<T> _lastSeenValue;
  
  public:
    InputToEventStream(Input<T>* wrappedInput)
    : _wrappedInput(wrappedInput)
    { }

    virtual void run() {
      T value = _wrappedInput->read();
      if (!_lastSeenValue.has_value() || value != _lastSeenValue.value()) {
        _lastSeenValue = value;
        EventStream<T>::_emit(value);
      }
    }
};

template <typename T>
class EventStreamFilter : public EventStream<T> {
  private:
    std::function<bool(Event<T>)> _filter;

  public:
    void receiveEvent(Event<T> event) {
      if (_filter(event)) {
        this->_emit(event);
      }
    }

    EventStreamFilter(EventStream<T>* wrappedEventStream, std::function<bool(Event<T>)> filter)
    : _filter(filter)
    {
      wrappedEventStream->registerSubscriber([this](Event<T> e) { this->receiveEvent(e); });
    }

    EventStreamFilter(EventStream<T>* wrappedEventStream, std::function<bool(T)> filter)
    : EventStreamFilter(wrappedEventStream, [filter](Event<T> e) { return filter(e.value); })
    { }
};

template <typename TIn, typename TOut>
class EventStreamTranslator : public EventStream<TOut> {
  private:
    const std::function<Event<TOut>(Event<TIn>)> _translator;

    void _receiveEvent(Event<TIn> event) {
      this->_emit(_translator(event));
    }

  public:
    EventStreamTranslator(EventStream<TIn>* wrappedEventStream, std::function<Event<TOut>(Event<TIn>)> translator)
    : _translator(translator)
    {
      wrappedEventStream->registerSubscriber([this](Event<TIn> e) { this->_receiveEvent(e); });
    }

    EventStreamTranslator(EventStream<TIn>* wrappedEventStream, std::function<TOut(TIn)> translator)
    : EventStreamTranslator(wrappedEventStream, [translator](Event<TIn> e) { return Event(e.timestamp, translator(e.value)); })
    { }
};

template <typename T>
class EventStreamNotEmpty : public EventStreamTranslator<std::optional<T>, T> {
  public:
    EventStreamNotEmpty(EventStream<std::optional<T>>* wrappedEventStream)
    :
      EventStreamTranslator<std::optional<T>, T>(
        new EventStreamFilter<std::optional<T>>(
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
class EventStreamDebouncer : public EventStream<T>, public Runnable {
  private:
    Timer _timer;
    std::optional<Event<T>> _firstEvent;
    std::optional<Event<T>> _latestEvent;

    void _receiveEvent(Event<T> event) {
      if (!_firstEvent.has_value()) {
        _firstEvent = event;
        _timer.restart();
      } else {
        _latestEvent = event;
      }
    }

  public:
    EventStreamDebouncer(EventStream<T>* wrappedEventStream, unsigned long delay)
    :
      _timer(Timer(
        delay,
        [this]() {
          if (!_latestEvent.has_value() || _latestEvent.value().value == _firstEvent.value().value) {
            // Event value(s) held through debounce. Emit the original one.
            this->_emit(_firstEvent.value());
          }
          // Reset the state either way, for the next debounce cycle.
          _firstEvent = _latestEvent = std::nullopt;
        },
        1,
        false,
        false
      ))
    {
      wrappedEventStream->registerSubscriber([this](Event<T> e) { this->_receiveEvent(e); });
    }

    void run() {
      _timer.run();
    }
};

template <typename TIndex, typename TEvent>
class EventStreamSwitcher : public EventStream<TEvent> {
  private:
    std::map<TIndex, EventStream<TEvent>*> _eventStreams;
    Input<TIndex>* _switchInput;
    std::string _message;

    void _receiveEventWithStreamIndex(TIndex index, Event<TEvent> event) {
      if (_switchInput->read() == index) {
        _message = "received event with stream index and it's the active stream";
        this->_emit(event);
      }
      _message = "received event with stream index and it ain't the active stream";
    }

  public:
    EventStreamSwitcher(std::map<TIndex, EventStream<TEvent>*> eventStreams, Input<TIndex>* switchInput)
    :
      _eventStreams(eventStreams),
      _switchInput(switchInput)
    {
      _message = "entering constructor";
      for (auto stream : _eventStreams) {
        stream.second->registerSubscriber([stream, this](Event<TEvent> e) { this->_receiveEventWithStreamIndex(stream.first, e); });
      }
      _message = "registered all the subscribers";
    }

    std::string getMessage() { return _message; }
};

#endif