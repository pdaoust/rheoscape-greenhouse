#ifndef PAULDAOUST_PROCESS_H
#define PAULDAOUST_PROCESS_H

#include <Controller.h>
#include <Range.h>
#include <Sensor.h>

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
    Sensor<T> _sensor;
    Controller<bool> _controller;
    Direction _controllerDirection;
    Range<T> _setpointRange;

  public:
    BangBangProcess(Sensor<T> sensor, Controller<bool> controller, Direction controllerDirection, Range<T> setpointRange)
    :
      _sensor(sensor),
      _controller(controller),
      _controllerDirection(controllerDirection),
      _setpointRange(setpointRange)
    { }

    void run() {
      T input = _sensor.read();
      if (input < _setpointRange.min) {
        // Tell the controller to attempt to increase the process variable if it's too low.
        // If the controller controls something that increases the process variable (e.g., heater, sprinkler), turn it on.
        // Otherwise (e.g., cooler), turn it off.
        _controller.setState(_controllerDirection == up);
      } else if (input > _setpointRange.max) {
        // Naturally, the opposite happens when it goes above the setpoint.
        _controller.setState(_controllerDirection == down);
      }
      // If we're between the two setpoints, don't change anything -- this has the effect of creating a dead band where the state stays the same as it was before.
    }

    void setSetpointRange(Range<T> range) {
      _setpointRange = range;
    }
};

#endif