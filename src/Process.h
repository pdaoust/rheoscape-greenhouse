#ifndef PAULDAOUST_PROCESS_H
#define PAULDAOUST_PROCESS_H

#include <Output.h>
#include <Range.h>
#include <Input.h>

class Process : public Runnable {
  public:
    enum Direction {
      up,
      down
    };
};

template <typename T>
class BangBangProcess : public Process {
  protected:
    Input<T> _input;
    Output<bool> _output;
    Direction _outputDirection;
    Range<T> _setpointRange;

  public:
    BangBangProcess(Input<T> input, Output<bool> output, Direction outputDirection, Range<T> setpointRange)
    :
      _input(input),
      _output(output),
      _outputDirection(outputDirection),
      _setpointRange(setpointRange)
    { }

    void run() {
      T input = _input.read();
      if (input < _setpointRange.min) {
        // Tell the output to attempt to increase the process variable if it's too low.
        // If the output controls something that increases the process variable (e.g., heater, sprinkler), turn it on.
        // Otherwise (e.g., cooler), turn it off.
        _output.write(_outputDirection == up);
      } else if (input > _setpointRange.max) {
        // Naturally, the opposite happens when it goes above the setpoint.
        _output.write(_outputDirection == down);
      }
      // If we're between the two setpoints, don't change anything -- this has the effect of creating a dead band where the state stays the same as it was before.
    }

    void setSetpointRange(Range<T> range) {
      _setpointRange = range;
    }
};

#endif