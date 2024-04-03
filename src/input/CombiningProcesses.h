#ifndef RHEOSCAPE_COMBINING_PROCESSES_H
#define RHEOSCAPE_COMBINING_PROCESSES_H

#include <stdexcept>
#include <iostream>

#include <input/Input.h>
#include <Range.h>

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
    std::vector<Input<TIn>*>* _inputs;
    std::function<TOut(TOut, TIn)> _foldFunction;
    Input<TOut>* _initialValueInput;

  public:
    FoldProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TOut, TIn)> foldFunction, Input<TOut>* initialValueInput)
    :
      _inputs(inputs),
      _foldFunction(foldFunction),
      _initialValueInput(initialValueInput)
    { }

    FoldProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TOut, TIn)> foldFunction, TOut initialValue)
    : FoldProcess(inputs, foldFunction, new ConstantInput<TOut>(initialValue))
    { }

    virtual TOut read() {
      TOut acc = _initialValueInput->read();
      for (uint i = 0; i < _inputs->size(); i ++) {
        acc = _foldFunction(acc, _inputs->at(i)->read());
      }
      return acc;
    }
};

template <typename T>
class ReduceProcess : public Input<T> {
  private:
    std::vector<Input<T>*>* _inputs;
    std::function<T(T, T)> _reduceFunction;

  public:
    ReduceProcess(std::vector<Input<T>*>* inputs, std::function<T(T, T)> reduceFunction)
    : 
      _inputs(inputs),
      _reduceFunction(reduceFunction)
    { }

    virtual T read() {
      if (_inputs->size() < 1) {
        throw std::invalid_argument("Can't reduce an empty vector");
      }
      T acc = _inputs->at(0)->read();
      for (uint i = 1; i < _inputs->size(); i ++) {
        acc = _reduceFunction(acc, _inputs->at(i)->read());
      }
      return acc;
    }
};

template <typename TIn, typename TOut>
class MapProcess : public Input<std::vector<TOut>> {
  private:
    std::vector<Input<TIn>*>* _inputs;
    std::function<TOut(TIn)> _mapFunction;

  public:
    MapProcess(std::vector<Input<TIn>*>* inputs, std::function<TOut(TIn)> mapFunction)
    :
      _inputs(inputs),
      _mapFunction(mapFunction)
    { }

    virtual std::vector<TOut> read() {
      std::vector<TOut> values;
      for (uint i = 0; i < _inputs->size(); i ++) {
        values.push_back(_mapFunction(_inputs->at(i)->read()));
      }
      return values;
    }
};

template <typename T>
class FilterProcess : public Input<std::vector<T>> {
  private:
    std::vector<Input<T>*>* _inputs;
    std::function<bool(T)> _filterFunction;
  
  public:
    FilterProcess(std::vector<Input<T>*>* inputs, std::function<bool(T)> filterFunction)
    :
      _inputs(inputs),
      _filterFunction(filterFunction)
    { }

    virtual std::vector<T> read() {
      std::vector<T> values;
      for (uint i = 0; i < _inputs->size(); i ++) {
        T value = _inputs->at(i)->read();
        if (_filterFunction(value)) {
          values.push_back(value);
        }
      }
      return values;
    }
};

template <typename T>
class AvgProcess : public Input<T> {
  private:
    std::vector<Input<T>*>* _inputs;
  
  public:
    AvgProcess(std::initializer_list<Input<T>*>* inputs)
    : AvgProcess((std::vector<Input<T>*>*) inputs)
    { }

    AvgProcess(std::vector<Input<T>*>* inputs)
    : _inputs(inputs) { }

    virtual T read() {
      T acc = 0;
      for (uint i = 0; i < _inputs->size(); i ++) {
        acc += _inputs->at(i)->read();
      }
      return acc / _inputs->size();
    }
};

template <typename T>
class MinProcess : public ReduceProcess<T>  {
  public:
    MinProcess(std::initializer_list<Input<T>*>* inputs)
    : MinProcess((std::vector<Input<T>*>*)inputs)
    { }

    MinProcess(std::vector<Input<T>*>* inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return std::min(acc, value); })
    { }
};

template <typename T>
class MaxProcess : public ReduceProcess<T>  {
  public:
    MaxProcess(std::initializer_list<Input<T>*>* inputs)
    : MaxProcess((std::vector<Input<T>*>*)inputs)
    { }

    MaxProcess(std::vector<Input<T>*>* inputs)
    : ReduceProcess<T>(inputs, [](T acc, T value) { return std::max(acc, value); })
    { }
};

template <typename TInputKey, typename TVal>
class InputSwitcher : public Input<TVal> {
  private:
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