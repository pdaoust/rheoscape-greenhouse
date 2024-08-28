#ifndef JSON_CONVERTERS_H
#define JSON_CONVERTERS_H

#include <optional>
#include <functional>
#include <ArduinoJson.hpp>
#include <Range.h>
#include <input/Input.h>
#include <input/TranslatingProcesses.h>
#include <input/GpioInputs.h>
#include <output/OutputFactories.h>
#include <event_stream/EventStream.h>
#include <GreenhouseState.h>

namespace ArduinoJson {
  template <typename T>
  struct Converter<Range<T>> {
    static void toJson(const Range<T>& src, JsonVariant dst) {
      dst["min"] = src.min;
      dst["max"] = src.max;
    }

    static Range<T> fromJson(JsonVariantConst src) {
      return Range<T>(src.as<JsonObjectConst>()["min"].as<T>(), src.as<JsonObjectConst>()["max"].as<T>());
    }

    static bool checkJson(JsonVariantConst src) {
      JsonObjectConst obj = src;
      bool result = obj;
      result &= obj.containsKey("min") && obj["min"].is<T>();
      result &= obj.containsKey("max") && obj["max"].is<T>();
      return result;
    }
  };

  template <typename T>
  struct Converter<SetpointAndHysteresis<T>> {
    static void toJson(const SetpointAndHysteresis<T>& src, JsonVariant dst) {
      dst["setpoint"] = src.setpoint;
      dst["hysteresis"] = src.hysteresis;
      dst["min"] = src.min();
      dst["max"] = src.max();
    }

    static SetpointAndHysteresis<T> fromJson(JsonVariantConst src) {
      return SetpointAndHysteresis<T>(src.as<JsonObjectConst>()["setpoint"].as<T>(), src.as<JsonObjectConst>()["hysteresis"].as<T>());
    }

    static bool checkJson(JsonVariantConst src) {
      JsonObjectConst obj = src;
      bool result = obj;
      result &= obj.containsKey("setpoint") && obj["setpoint"].is<T>();
      result &= obj.containsKey("hysteresis") && obj["hysteresis"].is<T>();
      return result;
    }
  };

  template <>
  struct Converter<TempUnit> {
    static void toJson(const TempUnit& src, JsonVariant dst) {
      switch (src) {
        case TempUnit::celsius:
          dst["display"] = "°C";
          dst["control"] = "celsius";
          break;
        case TempUnit::fahrenheit:
          dst["display"] = "°F";
          dst["control"] = "fahrenheit";
          break;
        case TempUnit::kelvin:
          dst["display"] = "K";
          dst["control"] = "kelvin";
          break;
      }
    }

    static TempUnit fromJson(JsonVariantConst src) {
      std::string value;
      if (src.is<JsonObjectConst>()) {
        value = src["control"].as<std::string>();
      } else {
        value = src.as<std::string>();
      }

      if (value == "celsius") {
        return TempUnit::celsius;
      } else if (value == "fahrenheit") {
        return TempUnit::fahrenheit;
      } else if (value == "kelvin") {
        return TempUnit::kelvin;
      }
    }

    static bool checkJson(JsonVariantConst src) {
      std::string value;
      if (src.is<std::string>()) {
        value = src.as<std::string>();
      } else if (src.is<JsonObjectConst>()) {
        if (!src["control"].is<std::string>()) {
          return false;
        }
        value = src["control"].as<std::string>();
      } else {
        return false;
      }

      return value == "celsius"
        || value == "fahrenheit"
        || value == "kelvin";
    }
  };

  template <typename T>
  struct Converter<std::optional<T>> {
    static void toJson(const std::optional<T>& src, JsonVariant dst) {
      if (src.has_value()) {
        dst.set(src.value());
      } else {
        dst.set(nullptr);
      }
    }

    static std::optional<T> fromJson(JsonVariantConst src) {
      if (src.isNull()) {
        return std::nullopt;
      }
      return std::optional<T>(src.as<T>());
    }

    static bool checkJson(JsonVariantConst src) {
      return src.isNull() || src.is<T>();
    }
  };

  template <typename T>
  struct Converter<TwoPointCalibration<T>> {
    static void toJson(const TwoPointCalibration<T>& src, JsonVariant dst) {
      dst["low_reference"] = src.lowReference;
      dst["low_raw"] = src.lowRaw;
      dst["high_reference"] = src.highReference;
      dst["high_raw"] = src.highRaw;
    }

    static TwoPointCalibration<T> fromJson(JsonVariantConst src) {
      return TwoPointCalibration<T>(src["low_reference"], src["low_raw"], src["high_reference"], src["high_raw"]);
    }

    static bool checkJson(JsonVariantConst src) {
      return src.is<JsonObjectConst>()
        && src["low_reference"].is<T>()
        && src["low_raw"].is<T>()
        && src["high_reference"].is<T>()
        && src["high_raw"].is<T>();
    }
  };
}

// These types don't make sense to convert from JSON.
template <typename T>
void convertToJson(const StateInput<T>& input, JsonVariant dest) {
  dest.set(input.read());
}

void convertToJson(const DoorState& value, JsonVariant dest) {
  switch (value) {
    case DoorState::doorClosed : dest.set("closed"); break;
    case DoorState::doorOpen : dest.set("open"); break;
  }
}

void convertToJson(const CoverAction& value, JsonVariant dest) {
  switch (value) {
    case CoverAction::closeCover: dest.set("closing/closed"); break;
    case CoverAction::openCover: dest.set("opening/open"); break;
  }
}

template <typename T>
void convertToJson(const EventStream<T>& eventStream, JsonVariant dest) {
  std::optional<Event<T>> maybeEvent = eventStream.getLastEvent();
  if (maybeEvent.has_value()) {
    dest.set(maybeEvent.value().value);
  }
}

template <typename T>
std::optional<T> unwrapEventStream(EventStream<T>* eventStream) {
  std::optional<Event<T>> value = eventStream->getLastEvent();
  if (value.has_value()) {
    return value.value().value;
  }
  return std::nullopt;
}

template <typename T>
std::optional<T> unwrapOptionalEventStream(EventStream<std::optional<T>>* eventStream) {
  std::optional<std::optional<T>> value = unwrapEventStream(eventStream);
  // These are two of the most hilarious lines of code I've written in a while.
  if (value.has_value() && value.value().has_value()) {
    return value.value().value();
  }
  return std::nullopt;
}

void convertToJson(GreenhouseState* const& ghState, JsonVariant dest) {
  dest["temp_unit"] = ghState->temp_unit->read();
  dest["shelf_temp"] = unwrapOptionalEventStream(ghState->shelf_temp);
  dest["shelf_temp_calibration"] = ghState->shelf_temp_calibration->read();
  dest["shelf_hum"] = unwrapOptionalEventStream(ghState->shelf_hum);
  dest["shelf_light"] = unwrapEventStream(ghState->shelf_light);
  dest["ground_temp"] = unwrapOptionalEventStream(ghState->ground_temp);
  dest["ground_temp_calibration"] = ghState->ground_temp_calibration->read();
  dest["ceiling_temp"] = unwrapOptionalEventStream(ghState->ceiling_temp);
  dest["ceiling_temp_calibration"] = ghState->ceiling_temp_calibration->read();
  dest["yuzu_temp"] = unwrapOptionalEventStream(ghState->yuzu_temp);
  dest["yuzu_temp_calibration"] = ghState->yuzu_temp_calibration->read();
  dest["fish_tank_temp"] = unwrapOptionalEventStream(ghState->fish_tank_temp);
  dest["fish_tank_temp_calibration"] = ghState->fish_tank_temp_calibration->read();
  dest["fan_status"] = unwrapEventStream(ghState->fan_status);
  dest["fan"] = ghState->fan->read();
  dest["heater_status"] = unwrapEventStream(ghState->heater_status);
  dest["heater"] = ghState->heater->read();
  dest["west_door_status"] = unwrapEventStream(ghState->west_door_status);
  dest["east_door_status"] = unwrapEventStream(ghState->east_door_status);
  dest["extreme_temp_alarm_control"] = ghState->extreme_temp_alarm_control->read();
  dest["door_alarm_control"] = ghState->door_alarm_control->read();
  dest["alarm_noise"] = ghState->alarm_noise->read();
  dest["alarm_phone"] = ghState->alarm_phone->read();
  dest["roof_vents_status"] = unwrapEventStream(ghState->roof_vents_status);
  dest["roof_vents_sensor_status"] = unwrapEventStream(ghState->roof_vents_sensor_status);
  dest["roof_vents"] = ghState->roof_vents->read();
  dest["mat_1_status"] = unwrapEventStream(ghState->mat_1_status);
  dest["mat_1_temp"] = unwrapOptionalEventStream(ghState->mat_1_temp);
  dest["mat_1_temp_calibration"] = ghState->shelf_temp_calibration->read();
  dest["mat_1"] = ghState->mat_1->read();
  dest["mat_2_status"] = unwrapEventStream(ghState->mat_2_status);
  dest["mat_2_temp"] = unwrapOptionalEventStream(ghState->mat_2_temp);
  dest["mat_2_temp_calibration"] = ghState->shelf_temp_calibration->read();
  dest["mat_2"] = ghState->mat_1->read();
}

#endif