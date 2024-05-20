#ifndef GREENHOUSE_STATE_H
#define GREENHOUSE_STATE_H

#include <input/Input.h>
#include <input/TranslatingProcesses.h>
#include <Range.h>

// Just cargo culting a singleton pattern from this SO answer:
// https://stackoverflow.com/a/1008289
class GreenhouseState {
  private:
    GreenhouseState();
    GreenhouseState(GreenhouseState const&);
    void operator=(GreenhouseState const&);
  
  public:
    StateInput<TempUnit>* temp_unit;
    EventStream<float>* shelf_temp;
    EventStream<float>* shelf_hum;
    EventStream<float>* shelf_light;
    EventStream<float>* ground_temp;
    EventStream<float>* ceil_temp;
    EventStream<float>* yuzu_temp;
    EventStream<bool>* fan_status;
    StateInput<SetpointAndHysteresis<float>>* fan_control;
    EventStream<bool>* heater_status;
    StateInput<SetpointAndHysteresis<float>>* heater_control;
    EventStream<bool>* west_door_status;
    EventStream<bool>* east_door_status;
    StateInput<Range<float>>* door_alarm_control;
    StateInput<bool>* door_alarm_noise;
    StateInput<std::string>* door_alarm_phone;
    EventStream<bool>* roof_vents_status;
    StateInput<SetpointAndHysteresis<float>>* roof_vents_control;
    EventStream<bool>* mat_1_status;
    StateInput<SetpointAndHysteresis<float>>* mat_1_control;
    EventStream<bool>* mat_2_status;
    StateInput<SetpointAndHysteresis<float>>* mat_2_control;

  static GreenhouseState& getInstance() {
    static GreenhouseState _instance;
    return _instance;
  }

  GreenhouseState(GreenhouseState const&) = delete;
  void operator=(GreenhouseState const&) = delete;
};

#endif