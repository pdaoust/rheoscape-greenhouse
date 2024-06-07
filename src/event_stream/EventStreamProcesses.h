#ifndef RHEOSCAPE_EVENT_STREAM_PROCESSES_H
#define RHEOSCAPE_EVENT_STREAM_PROCESSES_H

#include <functional>

#include <Runnable.h>
#include <Timer.h>
#include <input/Input.h>
#include <input/TranslatingProcesses.h>
#include <event_stream/EventStream.h>

template <typename T>
class InputToEventStream : public EventStream<T>, public Runnable {
  private:
    Input<T>* _wrappedInput;
    std::optional<T> _lastSeenValue;
    std::optional<Throttle<T>> _throttle;
    
  public:
    InputToEventStream(Input<T>* wrappedInput, unsigned long throttleRead = 0)
    :
      _wrappedInput(wrappedInput),
      _throttle(throttleRead
        ? (std::optional<Throttle<T>>)Throttle<T>(throttleRead, [wrappedInput]() { return wrappedInput->read(); })
        : (std::optional<Throttle<T>>)std::nullopt
      )
    { }

    virtual void run() {
      std::optional<T> nextValue;
      if (_throttle.has_value()) {
        nextValue = _throttle.value().tryRun();
      } else {
        nextValue = _wrappedInput->read();
      }

      if (!nextValue.has_value()) {
        return;
      }

      T nextValueValue = nextValue.value();
      if (!_lastSeenValue.has_value() || nextValueValue != _lastSeenValue.value()) {
        _lastSeenValue = nextValueValue;
        EventStream<T>::_emit(nextValueValue);
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

template <typename T>
class EventStreamCombiner : public EventStream<T> {
  public:
    EventStreamCombiner(std::vector<EventStream<T>*> eventStreams)
    {
      for (int i = 0; i < eventStreams.size(); i ++) {
        eventStreams[i]->registerSubscriber([this](Event<T> event) { this->_emit(event); });
      }
    }
};

template <typename T>
class Beacon : public EventStream<T>, public Runnable {
  private:
    Timer _timer;
    Input<bool>* _statusInput;

  public:
    Beacon(Input<T>* valueInput, Input<bool>* statusInput, unsigned long interval)
    :
      _statusInput(statusInput),
      _timer(Timer(
        interval,
        [valueInput, this]() {
          this->_emit(valueInput->read());
        },
        std::nullopt
        // We would emit an event on first run, but there's no event handler yet to listen to it.
      ))
    { }

    Beacon(Input<std::optional<T>>* valueInput, unsigned long interval)
    : Beacon(
      // FIXME: two memory leaks
      new TranslatingProcess<std::optional<T>, T>(valueInput, [](std::optional<T> value) { return value.value(); }),
      new TranslatingProcess<std::optional<T>, bool>(valueInput, [](std::optional<T> value) { return value.has_value(); }),
      interval
    )
    { }

    virtual void run() {
      if (_statusInput->read() && !_timer.isRunning()) {
        _timer.restart();
      } else if (!_statusInput->read() && _timer.isRunning()) {
        _timer.cancel();
      }
      _timer.run();
    }
};

#endif