#ifndef RHEOSCAPE_INPUT_H
#define RHEOSCAPE_INPUT_H

#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <variant>
#include <vector>

#include <helpers/string_format.h>
#include <Range.h>
#include <event_stream/EventStream.h>

template <typename T>
class Input {
  public:
    virtual T read() = 0;
};

template <typename T>
class FunctionInput : public Input<T> {
  private:
    std::function<T()> _computeValue;
  
  public:
    FunctionInput(std::function<T()> computeValue)
    : _computeValue(computeValue) { }

    virtual T read() { return _computeValue(); }
};

template <typename TChan, typename TVal>
class AbstractMultiInput {
  public:
    virtual TVal readChannel(TChan channel) = 0;
};

template <typename TChan, typename TVal>
class SingleChannelOfMultiInput : public Input<TVal> {
  private:
    AbstractMultiInput<TChan, TVal>* _wrappedInput;
    TChan _channel;

  public:
    SingleChannelOfMultiInput(AbstractMultiInput<TChan, TVal>* wrappedInput, TChan channel)
      :
        _wrappedInput(wrappedInput),
        _channel(channel)
      { }

    virtual TVal read() {
      return _wrappedInput->readChannel(_channel);
    }
};

template <typename TChan, typename TVal>
class MultiInput : public AbstractMultiInput<TChan, TVal> {
  public:
    SingleChannelOfMultiInput<TChan, TVal> getInputForChannel(TChan channel) {
      return SingleChannelOfMultiInput<TChan, TVal>(this, channel);
    }
};

// Lift a constant into an input.
template <typename T>
class ConstantInput : public Input<T> {
  private:
    T _value;

  public:
    ConstantInput(T value) : _value(value) { }

    virtual T read() {
      return _value;
    }
};

// Special case that gets used a lot.
template <typename T>
ConstantInput<Range<T>> makeRangeConstantInput(T min, T max) {
  return ConstantInput<Range<T>>(Range<T>{ min, max });
}

// A simple input that just returns whatever the value currently is at the pointer passed to it.
template <typename T>
class PointerInput : public Input<T> {
  private:
    T* _pointer;

  public:
    PointerInput(T* pointer) : _pointer(pointer) { }

    virtual T read() {
      return *_pointer;
    }
};

// A simple input whose value can be set.
template <typename T>
class StateInput : public Input<T>, public EventStream<T> {
  private:
    T _value;

  public:
    StateInput(T initialValue)
    : _value(initialValue)
    { }

    virtual T read() {
      return _value;
    }

    void write(T value) {
      _value = value;
      this->_emit(value);
    }
};

template <typename TChan, typename TVal>
class MapMultiInput : public MultiInput<TChan, TVal> {
  private:
    std::map<TChan, Input<TVal>*>* _inputs;
  
  public:
    MapMultiInput(std::map<TChan, Input<TVal>*>* inputs)
    : _inputs(inputs)
    { }

    virtual TVal readChannel(TChan channel) {
      if (!_inputs->contains(channel)) {
        std::__throw_invalid_argument(string_format("Channel %s doesn't exist", channel).c_str());
      }
      Input<TVal>* input = _inputs->at(channel);
      return input->read();
    }
};

#endif