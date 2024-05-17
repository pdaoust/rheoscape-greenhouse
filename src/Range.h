#ifndef RHEOSCAPE_RANGE_H
#define RHEOSCAPE_RANGE_H

template <typename T>
struct Range {
  T min;
  T max;

  Range(T min, T max) : min(min), max(max) { }

  operator SetpointAndHysteresis() const {
    return SetpointAndHysteresis(min + (max - min) / 2, (max - min) / 2);
  }
};

template <typename T>
struct SetpointAndHysteresis {
  T setpoint;
  // The distance that the measured value may diverge from the setpoint in either direction.
  T hysteresis;

  SetpointAndHysteresis(T setpoint, T hysteresis) : setpoint(setpoint), hysteresis(hysteresis) { }

  operator Range() const {
    return Range(setpoint - hysteresis, setpoint + hysteresis);
  }
};

#endif