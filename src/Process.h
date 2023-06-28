#ifndef PAULDAOUST_PROCESS_H
#define PAULDAOUST_PROCESS_H

#include <optional>
#include <Input.h>
#include <Range.h>
#include <Timer.h>

// A process isn't actually a thing unto itself.
// Instead, it's a pattern of taking one or more inputs, translating them,
// and implementing an input for the translated value.
// You can think of it as a mapper or reducer, but in class form.

template <typename TIn, typename TOut>
class TranslatingProcess : public Input<TOut> {
  private:
    Input<TIn> _wrappedInput;
    const std::function<std::optional<TOut>(std::optional<TIn>)>& _translator;

  public:
    TranslatingProcess(Input<TIn> wrappedInput, const std::function<std::optional<TOut>(std::optional<TIn>)>& translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator)
    { }

    std::optional<TOut> read() {
      return _translator(_wrappedInput.read());
    }
};

template <typename TIn, typename TOut>
class TranslatingNotEmptyProcess : public TranslatingProcess<TIn, TOut> {
  public:
    TranslatingNotEmptyProcess(Input<TIn> wrappedInput, const std::function<TOut(TIn)>& translator)
    : TranslatingProcess<TIn, TOut>(wrappedInput, [translator](std::optional<TIn> value) { return value.has_value() ? translator(value.value()) : value; })
    { }
};

// Take 'continuous' values and 'snap' them to a time interval.
template <typename TVal>
class TimeQuantisingProcess : public Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    RepeatTimer _timer;
    std::optional<TVal> _lastReadValue;
  
  public:
    TimeQuantisingProcess(Input<TVal> wrappedInput, unsigned long interval)
    :
      _wrappedInput(wrappedInput),
      _timer(RepeatTimer(
        millis(),
        interval,
        [this]() {
          _lastReadValue = _wrappedInput.read();
        }
      ))
    { }

    std::optional<TVal> read() {
      _timer.tick();
      return _lastReadValue;
    }
};

// Don't let a value change more frequently than every n milliseconds.
// Kinda like TimeQuantisingProcess, except it doesn't 'snap' to the interval.
template <typename TVal>
class ThrottlingProcess : public Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    unsigned long _minDelay;
    std::optional<TVal> _lastReadValue;
    Timer _timer;

  public:
    ThrottlingProcess(Input<TVal> wrappedInput, unsigned long minDelay)
    :
      _wrappedInput(wrappedInput),
      _minDelay(minDelay),
      _timer(Timer(0, [](){}))
    { }

    std::optional<TVal> read() {
      // Maybe I coulda figured out how to use my own Throttle
      // (which I suspect I built for this purpose) for this process,
      // but I couldn't figure out how to make it value-aware --
      // that is, to understand that only a changed value resets the throttle.
      if (_timer.isRunning()) {
        return _lastReadValue;
      }

      std::optional<TVal> value = _wrappedInput.read();
      if (value != _lastReadValue) {
        _timer.restart(millis() + _minDelay);
        _lastReadValue = value;
      }
      return _lastReadValue;
    }
};

// Convert a boolean input to another boolean input,
// where true is converted to a true/false pulse.
class BlinkingProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
    RepeatTimer _fullCycleTimer;
    Timer _offTimer;
    unsigned long _onTime;
  
  public:
    BlinkingProcess(Input<bool> wrappedInput, unsigned long onTime, unsigned long offTime)
    :
      _wrappedInput(wrappedInput),
      _onTime(onTime),
      _fullCycleTimer(RepeatTimer(
        0,
        onTime + offTime,
        [this]() {
          _offTimer.restart(millis() + _onTime);
        }
      )),
      _offTimer(Timer(
        0,
        [](){}
      ))
    { }

    std::optional<bool> read() {
      std::optional<bool> innerValue = _wrappedInput.read();
      if (innerValue) {
        if (!_fullCycleTimer.isRunning()) {
          _fullCycleTimer.restart(millis());
        }
        return _offTimer.isRunning();
      } else {
        _fullCycleTimer.cancel();
        _offTimer.cancel();
      }
    }
};

// Convert a 0..1 float to a boolean suitable for using in slow PWM outputs.
class SlowPwmProcess : public Input<bool> {
  private:
    RepeatTimer _timer;
    bool _currentValue;
  
  public:
    SlowPwmProcess(
      Input<float> wrappedInput,
      // The smallest discrete value in a cycle.
      unsigned long interval,
      // The number of intervals in a cycle. interval * resolution = cycle length.
      uint8_t resolution
    ) :
      _timer(RepeatTimer(
        millis(),
        interval,
        [this, &wrappedInput, resolution]() {
          float value = wrappedInput.read().value_or(0.0);
          unsigned long ticksPassed = _timer.getCount() % resolution;
          float dutyCycle = ticksPassed / resolution;
          _currentValue = ticksPassed <= value;
        }
      ))
    { }

    std::optional<bool> read() {
      _timer.tick();
      return _currentValue;
    }
};

// Boolean processes.
// These all cast empty values to false for the purpose of comparison.

class AndProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    AndProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    std::optional<bool> read() {
      return _wrappedInputA.read() && _wrappedInputB.read();
    }
};

class OrProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    OrProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    std::optional<bool> read() {
      return _wrappedInputA.read() || _wrappedInputB.read();
    }
};

class XorProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInputA;
    Input<bool> _wrappedInputB;
  
  public:
    XorProcess(Input<bool> wrappedInputA, Input<bool> wrappedInputB)
    :
      _wrappedInputA(wrappedInputA),
      _wrappedInputB(wrappedInputB)
    { }

    std::optional<bool> read() {
      // Gotta cast to boolean, because otherwise an empty value and a false will be equal.
      return !_wrappedInputA.read() != !_wrappedInputB.read();
    }
};

class NotProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
  
  public:
    NotProcess(Input<bool> wrappedInput) : _wrappedInput(wrappedInput) { }

    std::optional<bool> read() {
      return !_wrappedInput.read();
    }
};

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

// Add a delay into a input's reading,
// where it takes a given amount of time to reach the new reading.
// For example, for a hysteresiser that can only move 1 step per second,
// if the underlying input read 10 a second ago and now reads 20,
// it will actually read 11 now, 12 next second, and so forth
// until it finally reads 20 in 10 seconds.
// Of course, if the input goes back down to 10 the next second,
// it'll head towards that value.
template <typename TVal>
class HysteresisProcess : public Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    unsigned long _interval;
    TVal _stepsPerInterval;
    std::optional<TVal> _lastValue;
    unsigned long _lastRun;

  public:
    HysteresisProcess(Input<TVal> wrappedInput, unsigned long interval, TVal stepsPerInterval)
    :
      _wrappedInput(wrappedInput),
      _interval(interval),
      _stepsPerInterval(stepsPerInterval)
    { }
  
    std::optional<TVal> read() {
      std::optional<TVal> newValue = _wrappedInput.read();
      if (!newValue.has_value()) {
        return _lastValue;
      }

      unsigned long now = millis();
      if (_lastRun) {
        _lastValue = _lastValue + (float)(now - _lastRun) / _interval * _stepsPerInterval;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
    }
};

// Smooth a input reading over a moving average time interval in milliseconds,
// using the exponential moving average or single-pole IIR method.
template <typename TVal>
class ExponentialMovingAverageProcess : public Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    unsigned long _averageOver;
    std::optional<TVal> _lastValue;
    unsigned long _lastRun;
  
  public:
    ExponentialMovingAverageProcess(Input<TVal> wrappedInput, unsigned long averageOver)
    :
      _wrappedInput(wrappedInput),
      _averageOver(averageOver)
    { }

    std::optional<TVal> read() {
      std::optional<TVal> newValue = _wrappedInput.read();
      if (!newValue.has_value()) {
        return _lastValue;
      }

      unsigned long now = millis();
      if (_lastRun) {
        float intervalsSinceLastRun = (float)(now - _lastRun) / _averageOver;
        _lastValue -= _lastValue / intervalsSinceLastRun;
        _lastValue += newValue / intervalsSinceLastRun;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
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

// When the input goes true, emit true for a given number of milliseconds,
// then revert to false regardless of whether the input is true or false.
class TimedLatchProcess : public Input<bool> {
  private:
    Input<bool> _wrappedInput;
    Timer _timer;
    unsigned long _timeout;
    std::optional<bool> _previousValue;
  
  public:
    TimedLatchProcess(Input<bool> wrappedInput, unsigned long timeout)
    :
      _wrappedInput(wrappedInput),
      _timeout(timeout),
      _timer(Timer(
        0,
        []() {
          // We don't actually need a value;
          // we're just using this timer to test whether it's running.
        }
      ))
    { }

    std::optional<bool> read() {
      _timer.tick();
      if (_timer.isRunning()) {
        // Within the timer window.
        return true;
      }
      std::optional<bool> inputValue = _wrappedInput.read();
      if (!_previousValue && inputValue) {
        // 'rising edge'
        _timer.restart(millis() + _timeout);
        return true;
      } else {
        // Neither rising edge nor within the timer window.
        return false;
      }
    }
};

// Now we get to the control processes, which typically take a variable input
// and convert it into a control direction.

enum ProcessControlDirection {
  up,
  neutral,
  down
};

// This one is a simple bang-bang process, like a thermostat or moisture-controlled irrigation valve.
// It tries to drive the control up when the input is below the setpoint min,
// and tries to drive the control down when the input is above the setpoint max.
// In between the two, it tries to keep the control stable.
template <typename T>
class BangBangProcess : public Input<ProcessControlDirection> {
  protected:
    Input<T> _valueInput;
    Input<Range<std::optional<T>>> _setpointRangeInput;
    ProcessControlDirection _outputDirection;

  public:
    BangBangProcess(Input<T> valueInput, Input<Range<std::optional<T>>> setpointRangeInput)
    :
      _valueInput(valueInput),
      _setpointRangeInput(setpointRangeInput)
    { }

    std::optional<ProcessControlDirection> read() {
      std::optional<T> value = _valueInput.read();
      std::optional<Range<std::optional<T>>> setpointRange = _setpointRangeInput.read();
      if (!value.has_value() || !setpointRange.has_value()) {
        // Can't actually do anything with non-existent values.
        return std::nullopt;
      }

      if (setpointRange.value().min.has_value() && value.value() < setpointRange.value().min.value()) {
        // Tell the output to attempt to increase the process variable if it's too low.
        // If the output controls something that increases the process variable (e.g., heater, sprinkler), turn it on.
        // Otherwise (e.g., cooler), turn it off.
        return up;
      } else if (setpointRange.value().max.has_value() && value.value() < setpointRange.value().max.value()) {
        // Naturally, the opposite happens when it goes above the setpoint.
        return down;
      }
      return neutral;
    }
};

// This one converts a bang-bang input to a boolean, suitable for a switch.
// The behaviour when it receives a neutral input depends on its prior state.
// If it was previously up, it keeps it up, and vice versa.
class DirectionToBooleanProcess : public Input<bool> {
  private:
    Input<ProcessControlDirection> _wrappedInput;
    std::optional<ProcessControlDirection> _lastInputValue;
    bool _upIs;

    // We know we should never hit neutral, so disable the compiler warning.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wreturn-type"
    bool _processControlDirectionToBool(ProcessControlDirection direction) {
      switch (direction) {
        case up:
          return _upIs;
        case down:
          return !_upIs;
      }
    }
    #pragma GCC diagnostic pop

  public:
    DirectionToBooleanProcess(Input<ProcessControlDirection> wrappedInput, bool upIs)
    :
      _wrappedInput(wrappedInput),
      _upIs(upIs)
    { }

    std::optional<bool> read() {
      std::optional<ProcessControlDirection> inputValue = _wrappedInput.read();
      if (!inputValue.has_value()) {
        return std::nullopt;
      }

      switch (inputValue.value()) {
        case up:
        case down:
          _lastInputValue = inputValue;
          return _processControlDirectionToBool(inputValue.value());
        case neutral:
          if (!_lastInputValue.has_value()) {
            // In the case where _lastInputValue doesn't yet have a value,
            // it's not possible yet to determine whether neutral should hold up or down,
            // because we haven't seen an up or down yet.
            return std::nullopt;
          }
          return _processControlDirectionToBool(_lastInputValue.value());
        // Just doing this to shut the compiler up. It's actually unreachable.
        default: return std::nullopt;
      }
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