#ifndef RHEOSCAPE_COMBINING_PROCESSES_H
#define RHEOSCAPE_COMBINING_PROCESSES_H

#include <input/Input.h>

// Merges two inputs of the same type into one range input.
template <typename T>
class MergingRangeProcess : public BasicInput<Range<T>> {
  private:
    BasicInput<T> _inputMin;
    BasicInput<T> _inputMax;
  
  public:
    MergingRangeProcess(BasicInput<T> inputMin, BasicInput<T> inputMax)
    :
      _inputMin(inputMin),
      _inputMax(inputMax)
    { }

    Range<T> read() {
      return Range<T>(_inputMin.read(), _inputMax.read());
    }
};

// Merges two inputs into one tuple input.
template <typename T1, typename T2>
class Merging2Process : public BasicInput<std::tuple<T1, T2>> {
  private:
    BasicInput<T1> _input1;
    BasicInput<T2> _input2;
  
  public:
    Merging2Process(BasicInput<T1> input1, BasicInput<T2> input2)
    :
      _input1(input1),
      _input2(input2)
    { }

    std::tuple<T1, T2> read() {
      return std::tuple<T1, T2>(_input1.read(), _input2.read());
    }
};

template <typename T1, typename T2>
class Merging2NotEmptyProcess : public BasicInput<std::optional<std::tuple<T1, T2>>> {
  private:
    BasicInput<std::optional<T1>> _input1;
    BasicInput<std::optional<T2>> _input2;
  
  public:
    Merging2NotEmptyProcess(BasicInput<std::optional<T1>> input1, BasicInput<std::optional<T2>> input2)
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
class Merging3Process : public BasicInput<std::tuple<T1, T2, T3>> {
  private:
    BasicInput<T1> _input1;
    BasicInput<T2> _input2;
    BasicInput<T3> _input3;
  
  public:
    Merging3Process(BasicInput<T1> input1, BasicInput<T2> input2, BasicInput<T3> input3)
    :
      _input1(input1),
      _input2(input2),
      _input3(input3)
    { }

    std::tuple<T1, T2, T3> read() {
      return std::tuple<T1, T2, T3>(_input1.read(), _input2.read(), _input3.read());
    }
};

template <typename TIn, typename TOut>
class FoldProcess : public BasicInput<TOut> {
  private:
    std::vector<BasicInput<TIn>> _inputs;
    std::function<TOut(TOut, TIn)> _foldFunction;
    BasicInput<TOut> _initialValueInput;

  public:
    FoldProcess(std::vector<BasicInput<TIn>> inputs, std::function<TOut(TOut, TIn)> foldFunction, BasicInput<TOut> initialValueInput)
    :
      _inputs(inputs),
      _foldFunction(foldFunction),
      _initialValueInput(initialValueInput)
    { }

    FoldProcess(std::vector<BasicInput<TIn>> inputs, std::function<TOut(TOut, TIn)> foldFunction, TOut initialValue)
    : FoldProcess(inputs, foldFunction, BasicConstantInput<TOut>(initialValue))
    { }

    TOut read() {
      TOut acc = _initialValueInput.read();
      for (uint i = 0; i < _inputs.size(); i ++) {
        acc = _foldFunction(acc, _inputs[i].read());
      }
      return acc;
    }
};

template <typename T>
class ReduceProcess : public BasicInput<T> {
  private:
    std::vector<BasicInput<T>> _inputs;
    std::function<T(T, T)> _reduceFunction;

  public:
    ReduceProcess(std::vector<BasicInput<T>> inputs, std::function<T(T, T)> reduceFunction)
    : 
      _inputs(inputs),
      _reduceFunction(reduceFunction)
    {
      if (inputs.size() < 1) {
        throw std::invalid_argument("Can't reduce an empty vector");
      }
    }

    T read() {
      T acc = _inputs[0].read();
      for (uint i = 1; i < _inputs.size(); i ++) {
        acc = _reduceFunction(acc, _inputs[i].read());
      }
      return acc;
    }
};

template <typename TIn, typename TOut>
class MapProcess : public FoldProcess<TIn, std::vector<TOut>> {
  public:
    MapProcess(std::vector<BasicInput<TIn>> inputs, std::function<TOut(TIn)> mapFunction)
    : FoldProcess<TIn, std::vector<TOut>>(
      inputs,
      [mapFunction](std::vector<TOut> acc, TIn value) {
        acc.push_back(mapFunction(value));
        return acc;
      },
      // Create a new vector every time.
      // I don't know if this is necessary, and maybe it even makes things worse.
      BasicFunctionInput<std::vector<TOut>>([]() { return std::vector<TOut>(); })
    )
    { }
};

template <typename T>
class FilterProcess : public BasicInput<std::vector<T>> {
  private:
    std::vector<BasicInput<T>> _inputs;
    std::function<bool(T)> _filterFunction;
  
  public:
    FilterProcess(std::vector<BasicInput<T>> inputs, std::function<bool(T)> filterFunction)
    :
      _inputs(inputs),
      _filterFunction(filterFunction)
    { }

    std::vector<T> read() {
      std::vector<T> values;
      for (uint i = 0; i < _inputs.size(); i ++) {
        T value = values[i].read();
        if (_filterFunction(value)) {
          values.push_back(value);
        }
      }
      return values;
    }
};

template <typename T>
class AvgProcess : public ReduceProcess<T> {
  private:
    std::vector<BasicInput<T>> _inputs;
  
  public:
    AvgProcess(std::initializer_list<BasicInput<T>> inputs)
    : _inputs(inputs) { }

    AvgProcess(std::vector<BasicInput<T>> inputs)
    : _inputs(inputs) { }

    T read() {
      T acc = 0;
      for (uint i = 0; i < _inputs.size(); i ++) {
        acc += _inputs[i].read();
      }
      return acc / _inputs.size();
    }
};

template <typename T>
class MinProcess : public ReduceProcess<T>  {
  public:
    MinProcess(std::initializer_list<BasicInput<T>> inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return min(acc, value); })
    { }
};

template <typename T>
class MaxProcess : public ReduceProcess<T>  {
  public:
    MaxProcess(std::initializer_list<BasicInput<T>> inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return max(acc, value); })
    { }
};

template <typename TInputKey, typename TVal>
class InputSwitcher : public BasicInput<std::optional<TVal>> {
  private:
    std::map<TInputKey, BasicInput<TVal>> _inputs;
    BasicInput<TInputKey> _switchInput;
  
  public:
    InputSwitcher(std::map<TInputKey, BasicInput<TVal>> inputs, BasicInput<TInputKey> switchInput)
    : _inputs(inputs), _switchInput(switchInput)
    { }

    TVal read() {
      TInputKey currentInput = _switchInput.read();

      if (!_inputs.contains(currentInput.value())) {
        return std::nullopt;
      }

      return _inputs[currentInput.value()].read();
    }
};

template <typename T>
class NotEmptyFilterProcess : public MapProcess<std::optional<T>, T> {
  public:
    NotEmptyFilterProcess(std::vector<BasicInput<std::optional<T>>> inputs)
    : MapProcess<std::optional<T>, T>(
      FilterProcess<std::optional<T>>(
        inputs,
        [](std::optional<T> value) { return value.has_value(); }
      ),
      [](std::optional<T> value) { return value.value(); }
    )
    { }
};

#endif