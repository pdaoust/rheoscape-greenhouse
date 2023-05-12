#ifndef PAULDAOUST_RANGE_H
#define PAULDAOUST_RANGE_H

template <typename T>
struct Range {
  T min;
  T max;

  Range(T min, T max) : min(min), max(max) { }
};

#endif