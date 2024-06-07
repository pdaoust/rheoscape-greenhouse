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
const int ROOF_VENTS_OPEN_PIN = 0;
const int ROOF_VENTS_CLOSE_PIN = 0;
const int ROOF_VENTS_SENSOR_PIN = 0;
const int ONE_WIRE_PIN = 0;
const uint64_t MAT_1_THERM_ADDRESS = 0x0000000000000000;
const int MAT_1_CONTROL_PIN = 0;
const uint64_t MAT_2_THERM_ADDRESS = 0x0000000000000000;
const int MAT_2_CONTROL_PIN = 0;
const uint64_t YUZU_THERM_ADDRESS = 0x0000000000000000;
const uint64_t CEILING_THERM_ADDRESS = 0x0000000000000000;
const uint64_t GROUND_THERM_ADDRESS = 0x0000000000000000;
const int FANS_CONTROL_PIN = 0;
const int HEATER_CONTROL_PIN = 0;
const unsigned long BH1750_SAMPLE_INTERVAL = 1000;
const std::string MY_WIFI_AP_SSID = "logiehouse2";
const std::string MY_WIFI_AP_KEY = "";
const std::string TWILIO_ACCT_ID;
const std::string TWILIO_AUTH_TOKEN;
const std::string TWILIO_SENDER;

StateInput tempDisplayUnits(TempUnit::celsius);
TranslatingProcess<TempUnit, std::string> tempDisplayUnitsSymbol(&tempDisplayUnits, [](TempUnit value) { return displayUnit(value); });

SPIClass spi = SPI;
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
Merging2Process environmentMinMaxTemps(&environmentMinTemp, &environmentMaxTemp);

auto westDoorSensor = DoorSensor(0, INPUT_PULLDOWN);
DoorSensor eastDoorSensor(0, INPUT_PULLDOWN);

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

FunctionInput<std::optional<std::string>> doorAlarmThresholdMessageCreator(
  []() {
    Range<float> thresholds = doorAlarmThresholds.read();
    float min = environmentMinTemp.read();
    float max = environmentMaxTemp.read();
    std::tuple<bool, bool> anyDoorStateValue = anyDoorState.read();
    bool anyDoorIsOpen = std::get<0>(anyDoorStateValue);
    bool anyDoorIsClosed = std::get<1>(anyDoorStateValue);
    TempUnit tempUnit = tempDisplayUnits.read();
    std::string symbol = tempDisplayUnitsSymbol.read();

    if (min < thresholds.min && anyDoorIsOpen) {
      return (std::optional<std::string>)string_format("The min measured temperature has dropped %.1f%s below %.1f%s and at least one door is open", convertTempFromC(thresholds.min - min, tempUnit), symbol, convertTempFromC(thresholds.min, tempUnit), symbol);
    }
    if (max > thresholds.max && anyDoorIsClosed) {
      return (std::optional<std::string>)string_format("The max measured temperature has risen %.1f%s above %.1f%s and at least one door is closed", convertTempFromC(max - thresholds.max, tempUnit), symbol, convertTempFromC(thresholds.max, tempUnit), symbol);
    }
    return (std::optional<std::string>)std::nullopt;
  }
);
Beacon doorAlarmMessageEmitter(&doorAlarmThresholdMessageCreator, 1000 * 60 * 5);

FunctionInput<std::optional<std::string>> dangerAlarmThresholdMessageCreator(
  []() {
    Range<float> thresholds = dangerAlarmThresholds.read();
    float min = environmentMinTemp.read();
    float max = environmentMaxTemp.read();
    TempUnit tempUnit = tempDisplayUnits.read();
    std::string symbol = tempDisplayUnitsSymbol.read();

    if (min < thresholds.min) {
      return (std::optional<std::string>)string_format("DANGER!!! The min measured temperature has dropped %.1f%s below %.1f%s!", convertTempFromC(thresholds.min - min, tempUnit), symbol, convertTempFromC(thresholds.min, tempUnit), symbol);
    }
    if (max > thresholds.max) {
      return (std::optional<std::string>)string_format("DANGER!!! The max measured temperature has risen %.1f%s above %.1f%s!", convertTempFromC(max - thresholds.max, tempUnit), symbol, convertTempFromC(thresholds.max, tempUnit), symbol);
    }
    return (std::optional<std::string>)std::nullopt;
  }
);
Beacon dangerAlarmMessageEmitter(&dangerAlarmThresholdMessageCreator, 1000 * 60 * 5);

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

GreenhouseState ghState;

GreenhouseState initGreenhouseState() {
  GreenhouseState ghState;
  ghState.temp_unit = &tempDisplayUnits;
  ghState.shelf_temp = new InputToEventStream(new TemperatureTranslatingOptionalProcess(&shelfMaybeTempCalibrated, &tempDisplayUnits));
  ghState.shelf_temp_calibration = &shelfTempCalibration;
  ghState.shelf_hum = new InputToEventStream(&shelfMaybeHum);
  ghState.shelf_light = new InputToEventStream(&shelfLight);
  ghState.ground_temp = new InputToEventStream(new TemperatureTranslatingOptionalProcess(&groundMaybeTempCalibrated, &tempDisplayUnits));
  ghState.ground_temp_calibration = &groundTempCalibration;
  ghState.ceiling_temp = new InputToEventStream(new TemperatureTranslatingOptionalProcess(&ceilingMaybeTempCalibrated, &tempDisplayUnits));
  ghState.ceiling_temp_calibration = &ceilingTempCalibration;
  ghState.yuzu_temp = new InputToEventStream(new TemperatureTranslatingOptionalProcess(&yuzuMaybeTempCalibrated, &tempDisplayUnits));
  ghState.yuzu_temp_calibration = &yuzuTempCalibration;
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
  }
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