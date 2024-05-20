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
class OnePointCalibrationProcess : public TranslatingProcess<T, T> {
  public:
    OnePointCalibrationProcess(Input<T>* wrappedInput, Input<T>* offsetInput)
    : TranslatingProcess<T, T>(
      wrappedInput,
      [offsetInput](T value) { return value + offsetInput->read(); }
    )
    { }
};

template <typename T>
class OnePointCalibrationOptionalProcess : public TranslatingOptionalProcess<T, T> {
  public:
    OnePointCalibrationOptionalProcess(Input<std::optional<T>>* wrappedInput, Input<T>* offsetInput)
    : TranslatingOptionalProcess<T, T>(
      wrappedInput,
      [offsetInput](T value) { return value + offsetInput->read(); }
    )
    { }
};

template <typename TKey, typename TVal>
class OnePointCalibrationMultiProcess : public TranslatingMultiProcess<TKey, TVal, TVal> {
  public:
    OnePointCalibrationMultiProcess(MultiInput<TKey, TVal>* wrappedInput, MultiInput<TKey, TVal>* offsetInput)
    : TranslatingMultiProcess<TKey, TVal, TVal>(
      wrappedInput,
      [offsetInput](TVal value, TKey key) { return value + offsetInput->readChannel(key); }
    )
    { }
};

template <typename T>
T adjustToTwoPointCalibratedValue(T value, Range<T> referenceRange, Range<T> offsetRange) {
  return (((value - offsetRange.min) * (referenceRange.max - referenceRange.min)) / (offsetRange.max - offsetRange.min)) + referenceRange.min;
}

template <typename T>
class TwoPointCalibrationProcess : public TranslatingProcess<T, T> {
  public:
    TwoPointCalibrationProcess(Input<T>* wrappedInput, Input<Range<T>>* referenceRangeInput, Input<Range<T>>* offsetRangeInput)
    : TranslatingProcess<T, T>(
      wrappedInput,
      [referenceRangeInput, offsetRangeInput](T value) { return adjustToTwoPointCalibratedValue(value, referenceRangeInput->read(), offsetRangeInput->read()); }
    )
    { }
};

template <typename T>
class TwoPointCalibrationOptionalProcess : public TranslatingOptionalProcess<T, T> {
  public:
    TwoPointCalibrationOptionalProcess(Input<std::optional<T>>* wrappedInput, Input<Range<T>>* referenceRangeInput, Input<Range<T>>* offsetRangeInput)
    : TranslatingOptionalProcess<T, T>(
      wrappedInput,
      [referenceRangeInput, offsetRangeInput](T value) { return adjustToTwoPointCalibratedValue(value, referenceRangeInput->read(), offsetRangeInput->read()); }
    )
    { }
};

template <typename TKey, typename TVal>
class TwoPointCalibrationMultiProcess : public TranslatingMultiProcess<TKey, TVal, TVal> {
  public:
    TwoPointCalibrationMultiProcess(MultiInput<TKey, TVal>* wrappedInput, MultiInput<TKey, Range<TVal>>* referenceRangeMapInput, MultiInput<TKey, Range<TVal>>* offsetRangeMapInput)
    : TranslatingMultiProcess<TKey, TVal, TVal>(
      wrappedInput,
      [referenceRangeMapInput, offsetRangeMapInput](TVal value, TKey key) { return adjustToTwoPointCalibratedValue(value, referenceRangeMapInput->readChannel(key), offsetRangeMapInput->readChannel(key)); }
    )
    { }
};

enum TempUnit {
  celsius,
  fahrenheit,
  kelvin,
};

// Assumes a value in degrees Celsius, translating it to the proper temperature unit.
class TemperatureTranslatingProcess : public TranslatingProcess<float, float> {
  TemperatureTranslatingProcess(Input<float>* wrappedInput, Input<TempUnit>* tempUnitInput)
  : TranslatingProcess(
    wrappedInput,
    [tempUnitInput](float value) {
      switch (tempUnitInput->read()) {
        case TempUnit::celsius:
          return value;
        case TempUnit::fahrenheit:
          return value * (9.0f / 5.0f + 32);
        case TempUnit::kelvin:
          return value + 273.15f;
      }
    }
  )
};

#endif