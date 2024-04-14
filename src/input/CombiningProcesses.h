#ifndef RHEOSCAPE_COMBINING_PROCESSES_H
#define RHEOSCAPE_COMBINING_PROCESSES_H

#include <stdexcept>
#include <iostream>

#include <input/Input.h>
#include <Range.h>

// Convert a vector of inputs to an input of a vector.
template <typename T>
class InputOfInputs : public Input<std::vector<T>> {
  private:
    std::vector<Input<T>*>* _inputs;
  
  public:
    InputOfInputs(std::vector<Input<T>*>* inputs)
    : _inputs(inputs)
    { }

    virtual std::vector<T> read() {
      std::vector<T> values;
      for (uint i = 0; i < _inputs->size(); i ++) {
        values.push_back(_inputs->at(i)->read());
      }
      return values;
    }
};

template <typename TKey, typename TVal>
class InputOfMappedInputs : public Input<std::map<TKey, TVal>> {
  private:
    std::map<TKey, Input<TVal>*>* _inputs;
  
  public:
    InputOfMappedInputs(std::map<TKey, Input<TVal>*>* inputs)
    : _inputs(inputs)
    { }

    virtual std::map<TKey, TVal> read() {
      std::map<TKey, TVal> values;
      for (std::pair<TKey, Input<TVal>*> const kvp : *_inputs) {
        values[kvp.first] = kvp.second->read();
      }
      return values;
    }
};

// Merges two inputs of the same type into one range input.
template <typename T>
class MergingRangeProcess : public Input<Range<T>> {
  private:
    Input<T>* _inputMin;
    Input<T>* _inputMax;
  
  public:
    MergingRangeProcess(Input<T>* inputMin, Input<T>* inputMax)
    :
      _inputMin(inputMin),
      _inputMax(inputMax)
    { }

    virtual Range<T> read() {
      return Range<T>(_inputMin->read(), _inputMax->read());
    }
};

// Merges two inputs into one tuple input.
template <typename T1, typename T2>
class Merging2Process : public Input<std::tuple<T1, T2>> {
  private:
    Input<T1>* _input1;
    Input<T2>* _input2;
  
  public:
    Merging2Process(Input<T1>* input1, Input<T2>* input2)
    :
      _input1(input1),
      _input2(input2)
    { }

    virtual std::tuple<T1, T2> read() {
      return std::tuple<T1, T2>(_input1->read(), _input2->read());
    }
};

template <typename T1, typename T2>
class Merging2NotEmptyProcess : public Input<std::optional<std::tuple<T1, T2>>> {
  private:
    Input<std::optional<T1>>* _input1;
    Input<std::optional<T2>>* _input2;
  
  public:
    Merging2NotEmptyProcess(Input<std::optional<T1>>* input1, Input<std::optional<T2>>* input2)
    :
      _input1(input1),
      _input2(input2)
    { }

    virtual std::optional<std::tuple<T1, T2>> read() {
      std::optional<T1> value1 = _input1->read();
      std::optional<T2> value2 = _input2->read();
      if (!value1.has_value() || !value2.has_value()) {
        return std::nullopt;
      }

      return std::tuple<T1, T2>(value1.value(), value2.value());
    }
};

template <typename T1, typename T2, typename T3>
class Merging3Process : public Input<std::tuple<T1, T2, T3>> {
  private:
    Input<T1>* _input1;
    Input<T2>* _input2;
    Input<T3>* _input3;
  
  public:
    Merging3Process(Input<T1>* input1, Input<T2>* input2, Input<T3>* input3)
    :
      _input1(input1),
      _input2(input2),
      _input3(input3)
    { }

    virtual std::tuple<T1, T2, T3> read() {
      return std::tuple<T1, T2, T3>(_input1->read(), _input2->read(), _input3->read());
    }
};

template <typename TIn, typename TOut>
class FoldProcess : public Input<TOut> {
  private:
    Input<std::vector<TIn>>* _inputs;
    std::function<TOut(TOut, TIn)> _foldFunction;
    Input<TOut>* _initialValueInput;

  public:
    FoldProcess(Input<std::vector<TIn>>* inputs, std::function<TOut(TOut, TIn)> foldFunction, Input<TOut>* initialValueInput)
    :
      _inputs(inputs),
      _foldFunction(foldFunction),
      _initialValueInput(initialValueInput)
    { }

    FoldProcess(Input<std::vector<TIn>>* inputs, std::function<TOut(TOut, TIn)> foldFunction, TOut initialValue)
    : FoldProcess(inputs, foldFunction, new ConstantInput<TOut>(initialValue))
    { }

    FoldProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TOut, TIn)> foldFunction, Input<TOut>* initialValueInput)
    : FoldProcess(new InputOfInputs<TIn>(inputs), foldFunction, initialValueInput)
    { }

    FoldProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TOut, TIn)> foldFunction, TOut initialValue)
    : FoldProcess(new InputOfInputs<TIn>(inputs), foldFunction, new ConstantInput<TOut>(initialValue))
    { }

    virtual TOut read() {
      TOut acc = _initialValueInput->read();
      std::vector<TIn> inValues = _inputs->read();
      for (uint i = 0; i < inValues.size(); i ++) {
        acc = _foldFunction(acc, inValues[i]);
      }
      return acc;
    }
};

template <typename T>
class ReduceProcess : public Input<T> {
  private:
    Input<std::vector<T>>* _inputs;
    std::function<T(T, T)> _reduceFunction;

  public:
    ReduceProcess(std::vector<Input<T>*>* inputs, std::function<T(T, T)> reduceFunction)
    : ReduceProcess(new InputOfInputs(inputs), reduceFunction)
    { }

    ReduceProcess(Input<std::vector<T>>* inputs, std::function<T(T, T)> reduceFunction)
    :
      _inputs(inputs),
      _reduceFunction(reduceFunction)
    { }

    virtual T read() {
      std::vector<T> values = _inputs->read();
      if (values.size() < 1) {
        throw std::invalid_argument("Can't reduce an empty vector");
      }
      T acc = values.at(0);
      for (uint i = 1; i < values.size(); i ++) {
        acc = _reduceFunction(acc, values.at(i));
      }
      return acc;
    }
};

template <typename TIn, typename TOut>
class MapProcess : public Input<std::vector<TOut>> {
  private:
    Input<std::vector<TIn>>* _inputs;
    std::function<TOut(TIn)> _mapFunction;

  public:
    MapProcess(Input<std::vector<TIn>>* inputs, std::function<TOut(TIn)> mapFunction)
    :
      _inputs(inputs),
      _mapFunction(mapFunction)
    { }

    MapProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TIn)> mapFunction)
    : MapProcess(new InputOfInputs(inputs), mapFunction)
    { }

    virtual std::vector<TOut> read() {
      std::vector<TIn> values = _inputs->read();
      std::vector<TOut> mappedValues;
      for (uint i = 0; i < values.size(); i ++) {
        mappedValues.push_back(_mapFunction(values.at(i)));
      }
      return mappedValues;
    }
};

template <typename T>
class FilterProcess : public Input<std::vector<T>> {
  private:
    Input<std::vector<T>>* _inputs;
    std::function<bool(T)> _filterFunction;
  
  public:
    FilterProcess(Input<std::vector<T>>* inputs, std::function<bool(T)> filterFunction)
    :
      _inputs(inputs),
      _filterFunction(filterFunction)
    { }

    FilterProcess(std::vector<Input<T>*>* inputs, std::function<bool(T)> filterFunction)
    : FilterProcess(new InputOfInputs(inputs), filterFunction)
    { }

    virtual std::vector<T> read() {
      std::vector<T> values = _inputs->read();
      std::vector<T> filteredValues;
      for (uint i = 0; i < values.size(); i ++) {
        T value = values.at(i);
        if (_filterFunction(value)) {
          filteredValues.push_back(value);
        }
      }
      return filteredValues;
    }
};

template <typename T>
class AvgProcess : public Input<T> {
  private:
    Input<std::vector<T>>* _inputs;
  
  public:
    AvgProcess(Input<std::vector<T>>* inputs)
    : _inputs(inputs)
    { }

    AvgProcess(std::vector<Input<T>*>* inputs)
    : AvgProcess(new InputOfInputs(inputs))
    { }

    AvgProcess(std::initializer_list<Input<T>*>* inputs)
    : AvgProcess((std::vector<Input<T>*>*) inputs)
    { }

    virtual T read() {
      T acc = 0;
      std::vector<T> values = _inputs->read();
      for (uint i = 0; i < values.size(); i ++) {
        acc += values.at(i);
      }
      return acc / values.size();
    }
};

template <typename T>
class MinProcess : public ReduceProcess<T>  {
  public:
    MinProcess(Input<std::vector<T>>* inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return std::min(acc, value); })
    { }

    MinProcess(std::vector<Input<T>*>* inputs)
    : MinProcess(new InputOfInputs(inputs))
    { }

    MinProcess(std::initializer_list<Input<T>*>* inputs)
    : MinProcess((std::vector<Input<T>*>*)inputs)
    { }
};

template <typename T>
class MaxProcess : public ReduceProcess<T>  {
  public:
    MaxProcess(Input<std::vector<T>>* inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return std::max(acc, value); })
    { }
    
    MaxProcess(std::vector<Input<T>*>* inputs)
    : MaxProcess(new InputOfInputs(inputs))
    { }

    MaxProcess(std::initializer_list<Input<T>*>* inputs)
    : MaxProcess((std::vector<Input<T>*>*)inputs)
    { }
};

template <typename TInputKey, typename TVal>
class InputSwitcher : public Input<TVal> {
  private:
    // I'm not sure if it makes sense to store a map of inputs or an input of a map of values.
    std::map<TInputKey, Input<TVal>*>* _inputs;
    Input<TInputKey>* _switchInput;
  
  public:
    InputSwitcher(std::map<TInputKey, Input<TVal>*>* inputs, Input<TInputKey>* switchInput)
    : _inputs(inputs), _switchInput(switchInput)
    { }

    virtual TVal read() {
      TInputKey currentSwitchKey = _switchInput->read();
      return _inputs->at(currentSwitchKey)->read();
    }
};

template <typename T>
class NotEmptyProcess : public Input<std::vector<T>> {
  private:
    std::vector<Input<std::optional<T>>*>* _inputs;

  public:
    NotEmptyProcess(std::vector<Input<std::optional<T>>*>* inputs)
    : _inputs(inputs)
    { }

    virtual std::vector<T> read() {
      std::vector<T> values;
      for (uint i = 0; i < _inputs->size(); i ++) {
        std::optional<T> value = _inputs->at(i)->read();
        if (value.has_value()) {
          values.push_back(value.value());
        }
      }
      return values;
    }
};

#endif