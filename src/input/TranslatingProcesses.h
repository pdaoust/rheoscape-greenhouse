#ifndef RHEOSCAPE_TRANSLATING_PROCESSES_H
#define RHEOSCAPE_TRANSLATING_PROCESSES_H

#include <functional>

#include <input/Input.h>

template <typename TIn, typename TOut>
class TranslatingProcess : public Input<TOut> {
  private:
    Input<TIn>* _wrappedInput;
    std::function<TOut(TIn)> _translator;

  public:
    TranslatingProcess(Input<TIn>* wrappedInput, std::function<TOut(TIn)> translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    virtual TOut read() {
      return _translator(_wrappedInput->read());
    }
};

template <typename TIn, typename TOut>
class TranslatingOptionalProcess : public TranslatingProcess<std::optional<TIn>, std::optional<TOut>> {
  public:
    TranslatingOptionalProcess(
      Input<std::optional<TIn>>* wrappedInput,
      std::function<TOut(TIn)> translator
    )
    : TranslatingProcess<std::optional<TIn>, std::optional<TOut>>(
        wrappedInput,
        [translator](std::optional<TIn> value) {
          return value.has_value()
            ? (std::optional<TOut>)translator(value.value())
            : std::nullopt;
        })
    { }
};

template <typename TKey, typename TIn, typename TOut>
class TranslatingMultiProcess : public MultiInput<TKey, TOut> {
  private:
    MultiInput<TKey, TIn>* _wrappedProcess;
    std::function<TOut(TIn, TKey)> _translator;
  
  public:
    TranslatingMultiProcess(MultiInput<TKey, TIn>* wrappedProcess, std::function<TOut(TIn, TKey)> translator)
    :
      _wrappedProcess(wrappedProcess),
      _translator(translator)
    { }

    virtual TOut readChannel(TKey key) {
      return _translator(_wrappedProcess->readChannel(key), key);
    }
};

template <typename T>
class CalibrationProcess : public TranslatingProcess<T, T> {
  public:
    CalibrationProcess(Input<T>* wrappedInput, Input<T>* offsetInput)
    : TranslatingProcess<T, T>(
      wrappedInput,
      [offsetInput](T value) { return value + offsetInput->read(); }
    )
    { }
};

template <typename T>
class CalibrationOptionalProcess : public TranslatingOptionalProcess<T, T> {
  public:
    CalibrationOptionalProcess(Input<std::optional<T>>* wrappedInput, Input<T>* offsetInput)
    : TranslatingOptionalProcess<T, T>(
      wrappedInput,
      [offsetInput](T value) { return value + offsetInput->read(); }
    )
    { }
};

template <typename TKey, typename TVal>
class CalibrationMultiProcess : public TranslatingMultiProcess<TKey, TVal, TVal> {
  public:
    CalibrationMultiProcess(MultiInput<TKey, TVal>* wrappedInput, MultiInput<TKey, TVal>* offsetInput)
    : TranslatingMultiProcess<TKey, TVal, TVal>(
      wrappedInput,
      [offsetInput](TVal value, TKey key) { return value + offsetInput->readChannel(key); }
    )
    { }
};

#endif