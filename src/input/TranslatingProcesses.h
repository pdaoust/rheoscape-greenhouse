#ifndef RHEOSCAPE_TRANSLATING_PROCESSES_H
#define RHEOSCAPE_TRANSLATING_PROCESSES_H

#include <functional>

#include <helpers/temperature.h>
#include <input/Input.h>

// TODO: rename to FoldProcess, add ReduceProcess which takes the first value as the accumulator
template <typename TIn, typename TOut, typename TCtx>
class TranslatingProcessWithContext : public Input<TOut> {
  private:
    Input<TIn>* _wrappedInput;
    std::function<TOut(TIn, TCtx)> _translator;
    TCtx _context;

  public:
    TranslatingProcessWithContext(Input<TIn>* wrappedInput, std::function<TOut(TIn, TCtx)> translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    virtual TOut read() {
      return _translator(_wrappedInput->read(), _context);
    }
};

// TODO: rename to MapProcess
template <typename TIn, typename TOut>
class TranslatingProcess : public TranslatingProcessWithContext<TIn, TOut, bool> {
  public:
    TranslatingProcess(Input<TIn>* wrappedInput, std::function<TOut(TIn)> translator)
    : TranslatingProcessWithContext<TIn, TOut, bool>(wrappedInput, [translator](TIn value, bool _) { return translator(value); })
    { }
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
struct TwoPointCalibration {
  T lowReference;
  T lowRaw;
  T highReference;
  T highRaw;

  TwoPointCalibration(T lowReference, T lowRaw, T highReference, T highRaw)
  :
    lowReference(lowReference),
    lowRaw(lowRaw),
    highReference(highReference),
    highRaw(highRaw)
  { }

  T adjust(T value) {
    return (((value - lowRaw) * (highReference - lowReference)) / (highRaw - lowRaw)) + lowReference;
  }

  static TwoPointCalibration<T> waterReference() {
    return TwoPointCalibration<T>(0, 0, 100, 100);
  }
};

template <typename T>
class TwoPointCalibrationProcess : public TranslatingProcess<T, T> {
  public:
    TwoPointCalibrationProcess(Input<T>* wrappedInput, Input<TwoPointCalibration<T>>* calibrationInput)
    : TranslatingProcess<T, T>(
      wrappedInput,
      [calibrationInput](T value) { return calibrationInput->read().adjust(value); }
    )
    { }
};

template <typename T>
class TwoPointCalibrationOptionalProcess : public TranslatingOptionalProcess<T, T> {
  public:
    TwoPointCalibrationOptionalProcess(Input<std::optional<T>>* wrappedInput, Input<TwoPointCalibration<T>>* calibrationInput)
    : TranslatingOptionalProcess<T, T>(
      wrappedInput,
      [calibrationInput](T value) { return calibrationInput->read().adjust(value); }
    )
    { }
};

template <typename TKey, typename TVal>
class TwoPointCalibrationMultiProcess : public TranslatingMultiProcess<TKey, TVal, TVal> {
  public:
    TwoPointCalibrationMultiProcess(MultiInput<TKey, TVal>* wrappedInput, MultiInput<TKey, TwoPointCalibration<TVal>>* calibrationInput)
    : TranslatingMultiProcess<TKey, TVal, TVal>(
      wrappedInput,
      [calibrationInput](TVal value, TKey key) { return calibrationInput->readChannel(key).adjust(value); }
    )
    { }
};

// Assumes a value in degrees Celsius, translating it to the proper temperature unit.
class TemperatureTranslatingProcess : public TranslatingProcess<float, float> {
  public:
    TemperatureTranslatingProcess(Input<float>* wrappedInput, Input<TempUnit>* tempUnitInput)
    : TranslatingProcess(
      wrappedInput,
      [tempUnitInput](float value) { return convertTempFromC(value, tempUnitInput->read()); }
    )
    { }
};

class TemperatureTranslatingOptionalProcess : public TranslatingProcess<std::optional<float>, std::optional<float>> {
  public:
    TemperatureTranslatingOptionalProcess(Input<std::optional<float>>* wrappedInput, Input<TempUnit>* tempUnitInput)
    : TranslatingProcess(
      wrappedInput,
      [tempUnitInput](std::optional<float> value) { return value.has_value() ? std::optional(convertTempFromC(value.value(), tempUnitInput->read())) : std::nullopt; }
    )
    { }
};

template <typename T>
class SetpointAndHysteresisToRangeProcess : public TranslatingProcess<SetpointAndHysteresis<T>, Range<T>> {
  public:
    SetpointAndHysteresisToRangeProcess(Input<SetpointAndHysteresis<T>>* wrappedInput)
    : TranslatingProcess<SetpointAndHysteresis<T>, Range<T>>(
      wrappedInput,
      [](SetpointAndHysteresis<T> value) {
        return (Range<T>)value;
      }
    )
    { }
};

template <typename T>
class OptionalPinningProcess : public Input<T> {
  private:
    Input<std::optional<T>>* _wrappedInput;
    std::optional<T> _lastSeenValue;
    T _initialValue;
  
  public:
    OptionalPinningProcess(Input<std::optional<T>>* wrappedInput, T initialValue)
    :
      _wrappedInput(wrappedInput),
      _initialValue(initialValue)
    { }

    virtual T read() {
      std::optional<T> value = _wrappedInput->read();
      if (value.has_value()) {
        _lastSeenValue = value;
        return value.value();
      }
      if (_lastSeenValue.has_value()) {
        return _lastSeenValue.value();
      }
      return _initialValue;
    }
};

template <typename T1, typename TOptional>
class LiftToOptionalProcess : public Input<std::optional<T1>> {
  private:
    Input<T1>* _wrappedInput;
    Input<bool>* _optionalSwitchInput;

  public:
    LiftToOptionalProcess(Input<T1>* wrappedInput, Input<bool>* optionalSwitchInput)
    :
      _wrappedInput(wrappedInput),
      _optionalSwitchInput(optionalSwitchInput)
    { }

    LiftToOptionalProcess(Input<T1>* wrappedInput, Input<std::optional<TOptional>>* optionalSwitchInput)
    : LiftToOptionalProcess(wrappedInput, new TranslatingProcess<std::optional<TOptional>, bool>(optionalSwitchInput, [](std::optional<TOptional> value) { return value.has_value(); }))
    { }

    virtual std::optional<T1> read() {
      if (!_optionalSwitchInput->read()) {
        return std::nullopt;
      }

      return _wrappedInput->read();
    }
};

#endif