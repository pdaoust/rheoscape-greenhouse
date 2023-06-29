#ifndef RHEOSCAPE_CONTROL_PROCESSES_H
#define RHEOSCAPE_CONTROL_PROCESSES_H

#include <input/Input.h>

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

#endif