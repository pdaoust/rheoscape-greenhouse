#ifndef RHEOSCAPE_EVENT_STREAM_PROCESSES_H
#define RHEOSCAPE_EVENT_STREAM_PROCESSES_H

#include <input/Input.h>
#include <event_stream/EventStream.h>

template <typename T>
class InputToEventStream : public EventStream<T>, public Runnable {
  private:
    Input<T> _wrappedInput;
    T _lastSeenValue;
  
  public:
    InputToEventStream(Input<T> wrappedInput, Runner runner)
    :
      _wrappedInput(wrappedInput),
      Runnable(runner)
    { }

    void run() {
      T value = _wrappedInput.read();
      if (value != _lastSeenValue) {
        _lastSeenValue = value;
        EventStream<T>::_emit(value);
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
    Input<std::optional<T>> _wrappedInput;

  public:
    InputToEventStreamNotEmpty(Input<std::optional<T>> wrappedInput, Runner runner)
    :
      Runnable(runner),
      EventStreamNotEmpty<T>(
        InputToEventStream<std::optional<T>>(wrappedInput)
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
        EventStream<T>::_emit(_previousEvent.value());
      }
    }
};

template <typename TEvent, typename TIndex>
class EventStreamSwitcher : public EventStream<TEvent> {
  private:
    std::map<TIndex, EventStream<TEvent>> _eventStreams;
    Input<TIndex> _switchInput;

  public:
    EventStreamSwitcher(std::map<TIndex, EventStream<TEvent>> eventStreams, Input<TIndex> switchInput)
    :
      _eventStreams(eventStreams),
      _switchInput(switchInput)
    {
      for (std::pair<const TIndex, EventStream<TEvent>>& stream : _eventStreams) {
        stream.second.registerSubscriber([stream, this](Event<TEvent> v) { this->receiveEventWithStreamIndex(stream.first, v); });
      }
    }

    void receiveEventWithStreamIndex(TIndex index, Event<TEvent> event) {
      std::optional<TIndex> switchValue = _switchInput.read();
      if (switchValue.has_value() && switchValue.value() == index) {
        _emit(event);
      }
    }
};

#endif