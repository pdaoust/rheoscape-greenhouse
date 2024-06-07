#ifndef TEMP_UNIT_H
#define TEMP_UNIT_H

#include <string>

enum TempUnit {
  celsius,
  fahrenheit,
  kelvin,
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

std::string displayUnit(TempUnit tempUnit) {
  switch (tempUnit) {
    case TempUnit::celsius: return "°C";
    case TempUnit::fahrenheit: return "°F";
    case TempUnit::kelvin: return "K";
  }
}

float convertTempFromC(float tempC, TempUnit tempUnit) {
  switch (tempUnit) {
    case TempUnit::celsius:
      return tempC;
    case TempUnit::fahrenheit:
      return tempC * (9.0f / 5.0f + 32);
    case TempUnit::kelvin:
      return tempC + 273.15f;
  }
}

float convertTempToC(float temp, TempUnit tempUnit) {
  switch (tempUnit) {
    case TempUnit::celsius:
      return temp;
    case TempUnit::fahrenheit:
      return (temp - 32) * (5.0f / 9.0f);
    case TempUnit::kelvin:
      return temp - 273.15f;
  }
}

#pragma GCC diagnostic pop

#endif