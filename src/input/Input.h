#ifndef RHEOSCAPE_INPUT_H
#define RHEOSCAPE_INPUT_H

#include <map>
#include <optional>
#include <functional>
#include <variant>
#include <vector>

#include <Range.h>

template <typename T>
class Input {
  private:
    static T default_t;

  public:
    virtual T read() { return default_t; }
};

template <typename T>
class FunctionInput : Input<T> {
  private:
    std::function<T()> _computeValue;
  
  public:
    FunctionInput(std::function<T()> computeValue)
    : _computeValue(computeValue) { }

    virtual T read() { return _computeValue(); }
};

template <typename TChan, typename TVal>
class MultiInput {
  private:
    static TVal default_t;

  public:
    virtual TVal readChannel(TChan channel) { return default_t; };

    Input<TVal> getInputForChannel(TChan channel);
};

template <typename TChan, typename TVal>
class SingleChannelOfMultiInput : public Input<TVal> {
  private:
    MultiInput<TChan, TVal>& _wrappedInput;
    TChan _channel;

  public:
    SingleChannelOfMultiInput(MultiInput<TChan, TVal>& wrappedInput, TChan channel)
      :
        _wrappedInput(wrappedInput),
        _channel(channel)
      { }

    TVal read() {
      return _wrappedInput.readChannel(_channel);
    }
};

template <typename TChan, typename TVal>
Input<TVal> MultiInput<TChan, TVal>::getInputForChannel(TChan channel) {
  return SingleChannelOfMultiInput<TChan, TVal>(*this, channel);
}

// Lift a constant into an input.
template <typename T>
class ConstantInput : public Input<T> {
  private:
    T _value;

  public:
    ConstantInput(T value) : _value(value) { }

    T read() {
      return _value;
    }
};

// Special case that gets used a lot.
template <typename T>
Input<Range<T>> makeRangeConstantInput(T min, T max) {
  return ConstantInput<Range<T>>(Range<T>{ min, max });
}

// A simple input that just returns whatever the value currently is at the pointer passed to it.
template <typename T>
class PointerInput : public Input<T> {
  private:
    T* _pointer;

  public:
    PointerInput(T* pointer) : _pointer(pointer) { }

    std::optional<T> read() {
      return &_pointer;
    }
};

// A simple input whose value can be set.
template <typename T>
class StateInput : public Input<T> {
  private:
    T _value;

  public:
    T read() {
      return _value;
    }

    void write(T value) {
      _value = value;
    }
};

#endif