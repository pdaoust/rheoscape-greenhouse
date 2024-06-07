#ifndef GREENHOUSE_STATE_H
#define GREENHOUSE_STATE_H

#include <input/Input.h>
#include <input/TranslatingProcesses.h>
#include <input/GpioInputs.h>
#include <Range.h>
#include <output/OutputFactories.h>

// Just cargo culting a singleton pattern from this SO answer:
// https://stackoverflow.com/a/1008289
struct GreenhouseState {
  public:
    StateInput<TempUnit>* temp_unit;
    InputToEventStream<std::optional<float>>* shelf_temp;
    StateInput<TwoPointCalibration<float>>* shelf_temp_calibration;
    InputToEventStream<std::optional<float>>* shelf_hum;
    InputToEventStream<float>* shelf_light;
    InputToEventStream<std::optional<float>>* ground_temp;
    StateInput<TwoPointCalibration<float>>* ground_temp_calibration;
    InputToEventStream<std::optional<float>>* ceiling_temp;
    StateInput<TwoPointCalibration<float>>* ceiling_temp_calibration;
    InputToEventStream<std::optional<float>>* yuzu_temp;
    StateInput<TwoPointCalibration<float>>* yuzu_temp_calibration;
    InputToEventStream<bool>* fan_status;
    StateInput<SetpointAndHysteresis<float>>* fan;
    InputToEventStream<bool>* heater_status;
    StateInput<SetpointAndHysteresis<float>>* heater;
    InputToEventStream<DoorState>* west_door_status;
    InputToEventStream<DoorState>* east_door_status;
    StateInput<Range<float>>* extreme_temp_alarm_control;
    StateInput<Range<float>>* door_alarm_control;
    StateInput<bool>* alarm_noise;
    StateInput<std::string>* alarm_phone;
    InputToEventStream<CoverAction>* roof_vents_status;
    InputToEventStream<DoorState>* roof_vents_sensor_status;
    StateInput<SetpointAndHysteresis<float>>* roof_vents;
    InputToEventStream<bool>* mat_1_status;
    InputToEventStream<std::optional<float>>* mat_1_temp;
    StateInput<TwoPointCalibration<float>>* mat_1_temp_calibration;
    StateInput<SetpointAndHysteresis<float>>* mat_1;
    InputToEventStream<bool>* mat_2_status;
    InputToEventStream<std::optional<float>>* mat_2_temp;
    StateInput<TwoPointCalibration<float>>* mat_2_temp_calibration;
    StateInput<SetpointAndHysteresis<float>>* mat_2;
};

#endif