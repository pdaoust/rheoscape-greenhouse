#include <functional>
#include <memory>
#include <optional>
#include <sstream>

#ifdef PLATFORM_ARDUINO
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>

#include <Range.h>
#include <Runnable.h>
#include <input/Input.h>
#include <input/Ds18b20.h>
#include <input/Bme280.h>
#include <input/Bh1750.h>
#include <input/GpioInputs.h>
#include <input/TranslatingProcesses.h>
#include <input/CombiningProcesses.h>
#include <input/ControlProcesses.h>
#include <input/LogicalProcesses.h>
#include <output/OutputFactories.h>
#include <output/MotorDriver.h>
#include <notifier/TwilioMessageNotifier.h>
#include <event_stream/EventStreamProcesses.h>
#include <helpers/string_format.h>
#include <helpers/temperature.h>

#include <GreenhouseState.h>
#include <webServer.h>

// This is -1 kelvin, an impossible reading.
const float NO_READING_TEMP = -275.0f;
const int ONE_WIRE_PIN = 1;
const uint64_t MAT_1_THERM_ADDRESS = 0x0000000000000000;
const int MAT_1_CONTROL_PIN = 2;
const uint64_t MAT_2_THERM_ADDRESS = 0x0000000000000000;
const int MAT_2_CONTROL_PIN = 4;
const uint64_t YUZU_THERM_ADDRESS = 0x0000000000000000;
const uint64_t CEILING_THERM_ADDRESS = 0x0000000000000000;
const uint64_t GROUND_THERM_ADDRESS = 0x0000000000000000;
const uint64_t FISH_TANK_THERM_ADDRESS = 0x0000000000000000;
const int FANS_CONTROL_PIN = 5;
const int HEATER_CONTROL_PIN = 6;
const int ROOF_VENTS_OPEN_PIN = 14;
const int ROOF_VENTS_CLOSE_PIN = 15;
const int ROOF_VENTS_SENSOR_PIN = 16;
const int WEST_DOOR_SENSOR_PIN = 17;
const int EAST_DOOR_SENSOR_PIN = 18;
const int RED_LED_PIN = 35;
const int GREEN_LED_PIN = 36;
const int BLUE_LED_PIN = 37;
const int BUZZER_PIN = 38;
const unsigned long BH1750_SAMPLE_INTERVAL = 1000;
const std::string MY_WIFI_AP_SSID = "logiehouse2";
const std::string MY_WIFI_AP_KEY = "";
const std::string TWILIO_ACCT_ID;
const std::string TWILIO_AUTH_TOKEN;
const std::string TWILIO_SENDER;

StateInput tempDisplayUnits(TempUnit::celsius);
TranslatingProcess<TempUnit, std::string> tempDisplayUnitsSymbol(&tempDisplayUnits, [](TempUnit value) { return displayUnit(value); });

// This is probably FSPI, whose default pins are:
// COPI = 11
// SCK = 12
// CIPO = 13
SPIClass spi = SPI;
// Default pins are:
// SDA = 8
// SCL = 9
TwoWire i2c = Wire;
OneWire oneWire(ONE_WIRE_PIN);

Ds18b20 dallasTherms(&oneWire);

auto mat1MaybeTemp = dallasTherms.getInputForChannel(MAT_1_THERM_ADDRESS);
StateInput mat1TempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> mat1MaybeTempCalibrated(&mat1MaybeTemp, &mat1TempCalibration);
OptionalPinningProcess mat1Temp(&mat1MaybeTempCalibrated, NO_READING_TEMP);
StateInput mat1TempSetting(SetpointAndHysteresis(20.0f, 1.0f));
DirectionToBooleanProcess mat1Thermostat(new BangBangProcess<float>(&mat1Temp, &mat1TempSetting), true);
DigitalPinOutput mat1Control(MAT_1_CONTROL_PIN, HIGH, &mat1Thermostat);

auto mat2MaybeTemp = dallasTherms.getInputForChannel(MAT_2_THERM_ADDRESS);
StateInput mat2TempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> mat2MaybeTempCalibrated(&mat2MaybeTemp, &mat2TempCalibration);
OptionalPinningProcess mat2Temp(&mat2MaybeTempCalibrated, NO_READING_TEMP);
StateInput mat2TempSetting(SetpointAndHysteresis(20.0f, 1.0f));
DirectionToBooleanProcess mat2Thermostat(new BangBangProcess<float>(&mat2Temp, &mat2TempSetting), true);
DigitalPinOutput mat2Control(MAT_2_CONTROL_PIN, HIGH, &mat2Thermostat);

auto yuzuMaybeTemp = dallasTherms.getInputForChannel(YUZU_THERM_ADDRESS);
StateInput yuzuTempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> yuzuMaybeTempCalibrated(&yuzuMaybeTemp, &yuzuTempCalibration);
OptionalPinningProcess yuzuTemp(&yuzuMaybeTempCalibrated, NO_READING_TEMP);

auto groundMaybeTemp = dallasTherms.getInputForChannel(GROUND_THERM_ADDRESS);
StateInput groundTempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> groundMaybeTempCalibrated(&groundMaybeTemp, &groundTempCalibration);
OptionalPinningProcess groundTemp(&groundMaybeTempCalibrated, NO_READING_TEMP);

auto ceilingMaybeTemp = dallasTherms.getInputForChannel(CEILING_THERM_ADDRESS);
StateInput ceilingTempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> ceilingMaybeTempCalibrated(&ceilingMaybeTemp, &ceilingTempCalibration);
OptionalPinningProcess ceilingTemp(&ceilingMaybeTempCalibrated, NO_READING_TEMP);

auto fishTankMaybeTemp = dallasTherms.getInputForChannel(FISH_TANK_THERM_ADDRESS);
StateInput fishTankTempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> fishTankMaybeTempCalibrated(&fishTankMaybeTemp, &fishTankTempCalibration);
OptionalPinningProcess fishTankTemp(&fishTankMaybeTempCalibrated, NO_READING_TEMP);

Bme280 shelfSensor(&i2c);
auto shelfMaybeTemp = shelfSensor.getInputForChannel(Bme280Channel::tempC);
StateInput shelfTempCalibration(TwoPointCalibration<float>::waterReference());
TwoPointCalibrationOptionalProcess<float> shelfMaybeTempCalibrated(&shelfMaybeTemp, &shelfTempCalibration);
OptionalPinningProcess shelfTemp(&shelfMaybeTemp, NO_READING_TEMP);
auto shelfMaybeHum = shelfSensor.getInputForChannel(Bme280Channel::humidity);

Bh1750 shelfLight(BH1750_SAMPLE_INTERVAL, Bh1750::BH1750_ADDRESS_LOW, &i2c);

std::vector<Input<float>*> environmentTemps = { &yuzuTemp, &ceilingTemp, &shelfTemp };
AvgProcess environmentAvgTemp(&environmentTemps);
MaxProcess environmentMaxTemp(&environmentTemps);
MinProcess environmentMinTemp(&environmentTemps);
Merging2Process<float, float, Range<float>> environmentMinMaxTemps(&environmentMinTemp, &environmentMaxTemp, [](float min, float max) { return Range(min, max); });

auto westDoorSensor = DoorSensor(WEST_DOOR_SENSOR_PIN, INPUT_PULLDOWN);
DoorSensor eastDoorSensor(EAST_DOOR_SENSOR_PIN, INPUT_PULLDOWN);

StateInput roofVentsTempSetting(SetpointAndHysteresis(25.0f, 5.0f));
DirectionToBooleanProcess roofVentsThermostat(new BangBangProcess<float>(&environmentMaxTemp, &roofVentsTempSetting), true);
// temp up = true = open
TranslatingProcess<bool, CoverAction> roofVentsCoverControl(&roofVentsThermostat, [](bool value) { return value ? CoverAction::openCover : CoverAction::closeCover; });
MotorDriver roofVentsControl = makeCover(ROOF_VENTS_OPEN_PIN, ROOF_VENTS_CLOSE_PIN, LOW, 1000, &roofVentsCoverControl);
DoorSensor roofVentSensor(ROOF_VENTS_SENSOR_PIN, INPUT_PULLDOWN);

StateInput fanTempSetting(SetpointAndHysteresis(25.0f, 2.0f));
DirectionToBooleanProcess fanThermostat(new BangBangProcess<float>(&environmentAvgTemp, &fanTempSetting), true);
DigitalPinOutput fanControl(FANS_CONTROL_PIN, HIGH, &fanThermostat);

StateInput heaterTempSetting(SetpointAndHysteresis(20.0f, 5.0f));
DirectionToBooleanProcess heaterThermostat(new BangBangProcess<float>(&environmentAvgTemp, &heaterTempSetting), true);
DigitalPinOutput heaterControl(HEATER_CONTROL_PIN, HIGH, &heaterThermostat);

// Any door... including the roof vents
FunctionInput<std::tuple<bool, bool>> anyDoorState([]() {
  DoorState westDoorState = westDoorSensor.read();
  DoorState eastDoorState = westDoorSensor.read();
  DoorState roofVentState = westDoorSensor.read();
  return std::tuple(
    westDoorState == DoorState::doorOpen
    || eastDoorState == DoorState::doorOpen
    || roofVentState == DoorState::doorOpen,
    westDoorState == DoorState::doorClosed
    || eastDoorState == DoorState::doorClosed
    || roofVentState == DoorState::doorClosed
  );
});
StateInput doorAlarmThresholds(Range(10.0f, 25.0f));
StateInput dangerAlarmThresholds(Range(5.0f, 40.0f));
StateInput alarmNoise(true);
StateInput<std::string> alarmPhone("+12504865323");

// Return a negative value if the min temp is below the thresholds,
// a positive value if the max temp is above the thresholds,
// and nothing if it's within thresholds.
// The second tuple value is the threshold it's passed.
std::optional<std::tuple<float, float>> checkMinMaxAgainstThresholds(Range<float> minMax, Range<float> thresholds) {
  if (minMax.min >= thresholds.min && minMax.max <= thresholds.max) {
    return std::nullopt;
  }
  if (minMax.min < thresholds.min) {
    return std::tuple(minMax.min - thresholds.min, thresholds.min);
  }
  if (minMax.max > thresholds.max) {
    return std::tuple(minMax.max - thresholds.max, thresholds.max);
  }
}

Merging3TupleProcess<Range<float>, std::tuple<bool, bool>, Range<float>> environmentDoorStateAndDoorThresholds(
  &environmentMinMaxTemps,
  &anyDoorState,
  &doorAlarmThresholds
);

TranslatingProcess<std::tuple<Range<float>, std::tuple<bool, bool>, Range<float>>, std::optional<std::tuple<float, float>>> doorThresholdsCalculator(
  &environmentDoorStateAndDoorThresholds,
  [](std::tuple<Range<float>, std::tuple<bool, bool>, Range<float>> value) {
    auto stateVsThresholds = checkMinMaxAgainstThresholds(std::get<0>(value), std::get<2>(value));
    if (!stateVsThresholds.has_value()
      || (std::get<0>(std::get<1>(value)) && std::get<0>(stateVsThresholds.value()) < 1.0f)
      || (std::get<1>(std::get<1>(value)) && std::get<0>(stateVsThresholds.value()) < 1.0f)) {
      return stateVsThresholds;
    }
    // Outside thresholds, but not all doors are open/closed.
    // While this is a concern, we don't emit anything because
    // (a) it's not yet outside danger thresholds, and
    // (b) there's no door opening/closing action to take.
    return (std::optional<std::tuple<float, float>>)std::nullopt;
  }
);

TranslatingProcess<std::optional<std::tuple<float, float>>, std::optional<std::string>> doorAlarmThresholdMessageCreator(
  &doorThresholdsCalculator,
  [](std::optional<std::tuple<float, float>> value) {
    if (!value.has_value()) {
      return (std::optional<std::string>)std::nullopt;
    }

    float outOfRange = std::get<0>(value.value());
    float threshold = std::get<1>(value.value());
    TempUnit tempUnit = tempDisplayUnits.read();
    std::string symbol = tempDisplayUnitsSymbol.read();

    if (outOfRange < 0.0f) {
      return (std::optional<std::string>)string_format("The min measured temperature has dropped %.1f%s below %.1f%s and at least one door is open", convertTempFromC(-outOfRange, tempUnit), symbol, convertTempFromC(threshold, tempUnit), symbol);
    }
    if (outOfRange > 0.0f) {
      return (std::optional<std::string>)string_format("The max measured temperature has risen %.1f%s above %.1f%s and at least one door is closed", convertTempFromC(outOfRange, tempUnit), symbol, convertTempFromC(threshold, tempUnit), symbol);
    }
  }
);
Beacon doorAlarmMessageEmitter(&doorAlarmThresholdMessageCreator, 1000 * 60 * 5);
BlinkingProcess doorBuzzerBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&doorThresholdsCalculator, [](auto value) { return value.has_value(); }), 1000, 4000);
BlinkingProcess doorBlueLightBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&doorThresholdsCalculator, [](auto value) { return value.has_value() && std::get<0>(value.value()) < 0.0f; }), 1000, 4000);
BlinkingProcess doorRedLightBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&doorThresholdsCalculator, [](auto value) { return value.has_value() && std::get<0>(value.value()) > 0.0f; }), 1000, 4000);

Merging2TupleProcess<Range<float>, Range<float>> environmentAndDangerThresholds(
  &environmentMinMaxTemps,
  &dangerAlarmThresholds
);

TranslatingProcess<std::tuple<Range<float>, Range<float>>, std::optional<std::tuple<float, float>>> dangerThresholdsCalculator(
  &environmentAndDangerThresholds,
  [](std::tuple<Range<float>, Range<float>> value) {
    return checkMinMaxAgainstThresholds(std::get<0>(value), std::get<1>(value));
  }
);

TranslatingProcess<std::optional<std::tuple<float, float>>, std::optional<std::string>> dangerAlarmThresholdMessageCreator(
  &dangerThresholdsCalculator,
  [](std::optional<std::tuple<float, float>> value) {
    if (!value.has_value()) {
      return (std::optional<std::string>)std::nullopt;
    }

    float outOfRange = std::get<0>(value.value());
    float threshold = std::get<1>(value.value());
    TempUnit tempUnit = tempDisplayUnits.read();
    std::string symbol = tempDisplayUnitsSymbol.read();

    if (outOfRange < 0.0f) {
      return (std::optional<std::string>)string_format("DANGER! The min measured temperature has dropped %.1f%s below %.1f%s!!!", convertTempFromC(-outOfRange, tempUnit), symbol, convertTempFromC(threshold, tempUnit), symbol);
    }
    if (outOfRange > 0.0f) {
      return (std::optional<std::string>)string_format("DANGER! The max measured temperature has risen %.1f%s above %.1f%s!!!", convertTempFromC(outOfRange, tempUnit), symbol, convertTempFromC(threshold, tempUnit), symbol);
    }
  }
);
Beacon dangerAlarmMessageEmitter(&dangerAlarmThresholdMessageCreator, 1000 * 60 * 5);
BlinkingProcess dangerBuzzerBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&dangerThresholdsCalculator, [](auto value) { return value.has_value(); }), 1000, 500);
BlinkingProcess dangerBlueLightBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&dangerThresholdsCalculator, [](auto value) { return value.has_value() && std::get<0>(value.value()) < 0.0f; }), 1000, 500);
BlinkingProcess dangerRedLightBlinker(new TranslatingProcess<std::optional<std::tuple<float, float>>, bool>(&dangerThresholdsCalculator, [](auto value) { return value.has_value() && std::get<0>(value.value()) > 0.0f; }), 1000, 500);

std::vector<EventStream<std::string>*> alarmMessageEmitters = {
  &doorAlarmMessageEmitter,
  &dangerAlarmMessageEmitter
};
EventStreamCombiner alarmMessagesCombined(alarmMessageEmitters);

TranslatingProcess<std::string, TwilioConfig> twilioConfig(&alarmPhone, [](std::string phone) {
  return TwilioConfig {
    TWILIO_ACCT_ID,
    TWILIO_AUTH_TOKEN,
    TWILIO_SENDER,
    phone
  };
});

TwilioMessageNotifier alarmNotifier(&alarmMessagesCombined, &twilioConfig);

std::map<uint8_t, Input<bool>*> buzzerBlinkers = {
  { 0, &doorBuzzerBlinker },
  { 1, &dangerBuzzerBlinker }
};
FunctionInput<uint8_t> buzzerSwitch([]() {
  if (dangerThresholdsCalculator.read().has_value()) {
    return 1;
  } else {
    // This will also return 0 in the case of neither alarm blinker being on,
    // but that's okay, because the input stream will be false anyway.
    return 0;
  }
});
InputSwitcher<uint8_t, bool> buzzerSwitcher(&buzzerBlinkers, &buzzerSwitch);
DigitalPinOutput buzzer(BUZZER_PIN, HIGH, &buzzerSwitcher);

std::map<uint8_t, Input<bool>*> blueLightBlinkers = {
  { 0, &doorBlueLightBlinker },
  { 1, &dangerBlueLightBlinker }
};
FunctionInput<uint8_t> blueLightSwitch([]() {
  if (dangerThresholdsCalculator.read().has_value()) {
    return 1;
  } else {
    // This will also return 0 in the case of neither alarm blinker being on,
    // but that's okay, because the input stream will be false anyway.
    return 0;
  }
});
InputSwitcher<uint8_t, bool> blueLightSwitcher(&blueLightBlinkers, &blueLightSwitch);
DigitalPinOutput blueLight(BLUE_LED_PIN, HIGH, &blueLightSwitcher);

std::map<uint8_t, Input<bool>*> redLightBlinkers = {
  { 0, &doorRedLightBlinker },
  { 1, &dangerRedLightBlinker }
};
FunctionInput<uint8_t> redLightSwitch([]() {
  if (dangerThresholdsCalculator.read().has_value()) {
    return 1;
  } else {
    return 0;
  }
});
InputSwitcher<uint8_t, bool> redLightSwitcher(&redLightBlinkers, &redLightSwitch);
DigitalPinOutput redLight(RED_LED_PIN, HIGH, &redLightSwitcher);

BlinkingProcess heartbeat(new ConstantInput(true), 500, 5000);
StateInput faultState(false);
std::map<bool, Input<bool>*> greenLightInputs = {
  { false, &heartbeat },
  { true, new ConstantInput(false) }
};
InputSwitcher greenLightSwitcher(&greenLightInputs, &faultState);
DigitalPinOutput greenLight(GREEN_LED_PIN, HIGH, &greenLightSwitcher);

GreenhouseState ghState;

GreenhouseState initGreenhouseState() {
  GreenhouseState ghState;
  ghState.temp_unit = &tempDisplayUnits;
  ghState.shelf_temp = new InputToEventStream(&shelfMaybeTempCalibrated);
  ghState.shelf_temp_calibration = &shelfTempCalibration;
  ghState.shelf_hum = new InputToEventStream(&shelfMaybeHum);
  ghState.shelf_light = new InputToEventStream(&shelfLight);
  ghState.ground_temp = new InputToEventStream(&groundMaybeTempCalibrated);
  ghState.ground_temp_calibration = &groundTempCalibration;
  ghState.ceiling_temp = new InputToEventStream(&ceilingMaybeTempCalibrated);
  ghState.ceiling_temp_calibration = &ceilingTempCalibration;
  ghState.yuzu_temp = new InputToEventStream(&yuzuMaybeTempCalibrated);
  ghState.yuzu_temp_calibration = &yuzuTempCalibration;
  ghState.fish_tank_temp = new InputToEventStream(&fishTankMaybeTempCalibrated);
  ghState.fish_tank_temp_calibration = &fishTankTempCalibration;
  ghState.fan_status = new InputToEventStream(&fanThermostat);
  ghState.fan = &fanTempSetting;
  ghState.heater_status = new InputToEventStream(&heaterThermostat);
  ghState.heater = &heaterTempSetting;
  ghState.west_door_status = new InputToEventStream(&westDoorSensor);
  ghState.east_door_status = new InputToEventStream(&eastDoorSensor);
  ghState.extreme_temp_alarm_control = &dangerAlarmThresholds;
  ghState.door_alarm_control = &doorAlarmThresholds;
  ghState.alarm_noise = &alarmNoise;
  ghState.alarm_phone = &alarmPhone;
  ghState.roof_vents_status = new InputToEventStream(&roofVentsCoverControl);
  ghState.roof_vents_sensor_status = new InputToEventStream(&roofVentSensor);
  ghState.roof_vents = &roofVentsTempSetting;
  ghState.mat_1_status = new InputToEventStream(&mat1Thermostat);
  ghState.mat_1_temp = new InputToEventStream(&mat1MaybeTempCalibrated);
  ghState.mat_1_temp_calibration = &mat1TempCalibration;
  ghState.mat_1 = &mat1TempSetting;
  ghState.mat_2_status = new InputToEventStream(&mat2Thermostat);
  ghState.mat_2_temp = new InputToEventStream(&mat2MaybeTempCalibrated);
  ghState.mat_2_temp_calibration = &mat2TempCalibration;
  ghState.mat_2 = &mat2TempSetting;
  return ghState;
}

void registerRunnables(GreenhouseState* ghState) {
  Runner::registerRunnable(&mat1Control);
  Runner::registerRunnable(&mat2Control);
  Runner::registerRunnable(&roofVentsControl);
  Runner::registerRunnable(&fanControl);
  Runner::registerRunnable(&heaterControl);
  Runner::registerRunnable(&doorAlarmMessageEmitter);
  Runner::registerRunnable(&dangerAlarmMessageEmitter);
  Runner::registerRunnable(&buzzer);
  Runner::registerRunnable(&blueLight);
  Runner::registerRunnable(&redLight);
  Runner::registerRunnable(&greenLight);
  Runner::registerRunnable(ghState->shelf_temp);
  Runner::registerRunnable(ghState->shelf_hum);
  Runner::registerRunnable(ghState->shelf_light);
  Runner::registerRunnable(ghState->ground_temp);
  Runner::registerRunnable(ghState->ceiling_temp);
  Runner::registerRunnable(ghState->yuzu_temp);
  Runner::registerRunnable(ghState->fan_status);
  Runner::registerRunnable(ghState->heater_status);
  Runner::registerRunnable(ghState->west_door_status);
  Runner::registerRunnable(ghState->east_door_status);
  Runner::registerRunnable(ghState->roof_vents_status);
  Runner::registerRunnable(ghState->roof_vents_sensor_status);
  Runner::registerRunnable(ghState->mat_1_status);
  Runner::registerRunnable(ghState->mat_1_temp);
  Runner::registerRunnable(ghState->mat_2_status);
  Runner::registerRunnable(ghState->mat_2_temp);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Getting started!");
  Serial.println("Connecting to WiFi...");
  WiFi.begin(MY_WIFI_AP_SSID.c_str(), MY_WIFI_AP_KEY.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    faultState.write(true);
  }
  faultState.write(false);
  Serial.println("");
  Serial.print("Connected to WiFi network with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting up web server...");
  ghState = initGreenhouseState();
  setupWebServer(&ghState);
  Serial.println("Web server started!");
  registerRunnables(&ghState);
}

void loop() {
  Runner::run();
}

#else

int main() {
  return 0;
}

#endif