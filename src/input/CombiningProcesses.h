#ifndef RHEOSCAPE_COMBINING_PROCESSES_H
#define RHEOSCAPE_COMBINING_PROCESSES_H

#include <input/Input.h>

// Merges two inputs of the same type into one range input.
template <typename T>
class MergingRangeProcess : public Input<Range<std::optional<T>>> {
  private:
    Input<T> _inputMin;
    Input<T> _inputMax;
  
  public:
    MergingRangeProcess(Input<T> inputMin, Input<T> inputMax)
    :
      _inputMin(inputMin),
      _inputMax(inputMax)
    { }

    std::optional<Range<std::optional<T>>> read() {
      return Range<T>(_inputMin.read(), _inputMax.read());
    }
};

// Merges two inputs into one tuple input.
template <typename T1, typename T2>
class Merging2Process : public Input<std::tuple<std::optional<T1>, std::optional<T2>>> {
  private:
    Input<T1> _input1;
    Input<T2> _input2;
  
  public:
    Merging2Process(Input<T1> input1, Input<T2> input2)
    :
      _input1(input1),
      _input2(input2)
    { }

    std::optional<std::tuple<std::optional<T1>, std::optional<T2>>> read() {
      return std::tuple<T1, T2>(_input1.read(), _input2.read());
    }
};

template <typename T1, typename T2>
class Merging2NotEmptyProcess : public Input<std::tuple<T1, T2>> {
  private:
    Input<T1> _input1;
    Input<T2> _input2;
  
  public:
    Merging2NotEmptyProcess(Input<T1> input1, Input<T2> input2)
    :
      _input1(input1),
      _input2(input2)
    { }

    std::optional<std::tuple<T1, T2>> read() {
      std::optional<T1> value1 = _input1.read();
      std::optional<T2> value2 = _input2.read();
      if (!value1.has_value() || !value2.has_value()) {
        return std::nullopt;
      }

      return std::tuple<T1, T2>(value1.value(), value2.value());
    }
};

template <typename T1, typename T2, typename T3>
class Merging3Process : public Input<std::tuple<std::optional<T1>, std::optional<T2>, std::optional<T3>>> {
  private:
    Input<T1> _input1;
    Input<T2> _input2;
    Input<T3> _input3;
  
  public:
    Merging3Process(Input<T1> input1, Input<T2> input2, Input<T3> input3)
    :
      _input1(input1),
      _input2(input2),
      _input3(input3)
    { }

    std::optional<std::tuple<std::optional<T1>, std::optional<T2>, std::optional<T3>>> read() {
      return std::tuple<std::optional<T1>, std::optional<T2>, std::optional<T3>>(_input1.read(), _input2.read(), _input3.read());
    }
};

enum AggregationType {
  average,
  maximum,
  minimum
};

template <typename TVal>
class AggregatingProcess : public MultiInput<AggregationType, TVal> {
  private:
    std::vector<Input<TVal>> _inputs;
  
  public:
    AggregatingProcess(std::vector<Input<TVal>> inputs)
    : _inputs(inputs)
    { }

    std::optional<TVal> readChannel(AggregationType channel) {
      std::optional<TVal> acc;
      switch (channel) {
        case average:
          uint count;
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              count ++;
              acc = acc.has_value() ? acc.value() + value.value() : value;
            }
          }
          acc = acc.has_value() ? acc.value() / count : acc;
          break;
        case minimum:
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              acc = acc.has_value() ? min(acc.value(), value.value()) : value;
            }
          }
          break;
        case maximum:
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              acc = acc.has_value() ? max(acc.value(), value.value()) : value;
            }
          }
          break;
      }

      return acc;
    }
};

template <typename TInputKey, typename TVal>
class InputSwitcher : public Input<TVal> {
  private:
    std::map<TInputKey, Input<TVal>> _inputs;
    Input<TInputKey> _switchInput;
  
  public:
    InputSwitcher(std::map<TInputKey, Input<TVal>> inputs, Input<TInputKey> switchInput)
    : _inputs(inputs), _switchInput(switchInput)
    { }

    std::optional<TVal> read() {
      std::optional<TInputKey> currentInput = _switchInput.read();
      if (!currentInput.has_value()) {
        return std::nullopt;
      }

      if (!_inputs.contains(currentInput.value())) {
        return std::nullopt;
      }

      return _inputs[currentInput.value()].read();
    }
};

template <typename TVal>
class InputAggregator : public Input<std::vector<TVal>> {
  private:
    std::vector<Input<TVal>> _inputs;
  
  public:
    InputAggregator(std::initializer_list<Input<TVal>> inputs)
    : _inputs(inputs) { }

    std::optional<std::vector<TVal>> read() {
      std::vector<TVal> values;
      for (Input<TVal> input : _inputs) {
        std::optional<TVal> value = input.read();
        if (value.has_value()) {
          values.push_back(value.value());
        }
      }
      if (values.size()) {
        return values;
      } else {
        return std::nullopt;
      }
    }
};

#endif