#ifndef RHEOSCAPE_RANGE_H
#define RHEOSCAPE_RANGE_H

template <typename T>
struct Range {
  T min;
  T max;

  Range(T min, T max) : min(min), max(max) { }
};

#endif