#include <optional>
#include <sstream>

#include <Arduino.h>
#include <LiquidCrystal.h>

#include <Input.h>
#include <Output.h>
#include <Process.h>
#include <Range.h>
#include <Runnable.h>
#include <Timer.h>

#define SPI_CIPO_PIN 0
#define SPI_PICO_PIN 0
#define I2C_SCL_PIN 16
#define I2C_SDA_PIN 17
#define SHT21_DATA_PIN 0
#define SHT21_CLOCK_PIN 0
#define SHT21_TEMP_CALIBRATION_OFFSET 0.0
// FIXME: real value please.
#define ONEWIRE_DATA_PIN 0
#define DS18B20_HEATMAT_1_ADDRESS 0x0000000000000000
#define DS18B20_HEATMAT_1_CALIBRATION_OFFSET 0.0
#define DS18B20_HEATMAT_2_ADDRESS 0x0000000000000000
#define DS18B20_HEATMAT_2_CALIBRATION_OFFSET 0.0
#define DS18B20_GND_W_ADDRESS 0x0000000000000000
#define DS18B20_GND_W_CALIBRATION_OFFSET 0.0
#define DS18B20_CEIL_W_ADDRESS 0x0000000000000000
#define DS18B20_CEIL_W_CALIBRATION_OFFSET 0.0
#define DS18B20_GND_E_ADDRESS 0x0000000000000000
#define DS18B20_GND_E_CALIBRATION_OFFSET 0.0
#define DS18B20_CEIL_E_ADDRESS 0x0000000000000000
#define DS18B20_CEIL_E_CALIBRATION_OFFSET 0.0
// FIXME: real value please.
#define DOOR_W_SENSOR_PIN 0
// FIXME: real value please.
#define DOOR_E_SENSOR_PIN 0
// FIXME: real value please.
#define VENTS_SENSOR_PIN 0

const TwilioConfig twilioConfig{
  "accountId",
  "authToken",
  "sender",
  "+12504865323"
};
// Every ten minutes seems good.
#define DOOR_ALARM_MESSAGE_SEND_EVERY 1000 * 60 * 10

#define HD44780_RS_PIN 17
#define HD44780_EN_PIN 10
#define HD44780_D1_PIN 11
#define HD44780_D2_PIN 12
#define HD44780_D3_PIN 13
#define HD44780_D4_PIN 14

#define HEATMAT_1_RELAY_PIN 39
#define HEATMAT_2_RELAY_PIN 40
#define HEATER_RELAY_PIN 41
#define FAN_RELAY_PIN 42
#define FAN_CYCLE_TIME INDUCTIVE_LOAD_CYCLE_TIME
// FIXME: real value please.
#define WATER_RELAY_PIN 0
// FIXME: real value please.
#define ROOF_VENT_ACTUATOR_CLOSE_PIN 0
// FIXME: real value please.
#define ROOF_VENT_ACTUATOR_OPEN_PIN 0
// A one-hour cycle is probably granular enough for roof vents.
// Maybe it needs to be even coarser?
#define ROOF_VENT_ACTUATION_TIME 1000 * 60 * 60

Runner runner(1);
Ds18b20 oneWireTempSensors(
  ONEWIRE_DATA_PIN,
  Ds18b20::half_degree,
  std::map<uint64_t, float>{
    { DS18B20_HEATMAT_1_ADDRESS, DS18B20_HEATMAT_1_CALIBRATION_OFFSET },
    { DS18B20_HEATMAT_2_ADDRESS, DS18B20_HEATMAT_2_CALIBRATION_OFFSET },
    { DS18B20_GND_W_ADDRESS, DS18B20_GND_W_CALIBRATION_OFFSET },
    { DS18B20_GND_E_ADDRESS, DS18B20_GND_E_CALIBRATION_OFFSET },
    { DS18B20_CEIL_W_ADDRESS, DS18B20_CEIL_W_CALIBRATION_OFFSET },
    { DS18B20_CEIL_E_ADDRESS, DS18B20_CEIL_E_CALIBRATION_OFFSET }
  }
);

// Heat mats setup
Input<float> heatmat1Sensor = oneWireTempSensors.getInputForChannel(DS18B20_HEATMAT_1_ADDRESS);
auto heatmat1RangeInput = makeRangeConstantInput<float>(24.0f, 26.0f);
Input<ProcessControlDirection> heatmat1BangBang = BangBangProcess<float>(heatmat1Sensor, heatmat1RangeInput);
Input<bool> heatmat1RelayInput = DirectionToBooleanProcess(heatmat1BangBang, true);
Output heatmat1Controller = makeRelay(
  HEATMAT_1_RELAY_PIN,
  LOW,
  MECHANICAL_RELAY_CYCLE_TIME,
  heatmat1RelayInput,
  runner
);
Input<float> heatmat2Sensor = oneWireTempSensors.getInputForChannel(DS18B20_HEATMAT_2_ADDRESS);
auto heatmat2RangeInput = ConstantInput<Range<std::optional<float>>>(Range<std::optional<float>>{ 24.0, 26.0 });
Input<ProcessControlDirection> heatmat2BangBang = BangBangProcess<float>(heatmat2Sensor, heatmat2RangeInput);
Input<bool> heatmat2RelayInput = DirectionToBooleanProcess(heatmat2BangBang, true);
Output heatmat2Controller = makeRelay(
  HEATMAT_2_RELAY_PIN,
  LOW,
  MECHANICAL_RELAY_CYCLE_TIME,
  heatmat2RelayInput,
  runner
);

// Next, the general condition of the greenhouse, which will get fed into various processes.
Input<float> groundWestSensor = oneWireTempSensors.getInputForChannel(DS18B20_GND_W_ADDRESS);
Input<float> groundEastSensor = oneWireTempSensors.getInputForChannel(DS18B20_GND_E_ADDRESS);
Input<float> ceilingWestSensor = oneWireTempSensors.getInputForChannel(DS18B20_CEIL_W_ADDRESS);
Input<float> ceilingEastSensor = oneWireTempSensors.getInputForChannel(DS18B20_CEIL_E_ADDRESS);
Sht21 greenhouseTempHum(SHT21_DATA_PIN, SHT21_CLOCK_PIN, SHT21_TEMP_CALIBRATION_OFFSET);
Input<float> greenhouseTemp = greenhouseTempHum.getInputForChannel(Sht21Channel::tempC);
Input<float> greenhouseHum = greenhouseTempHum.getInputForChannel(Sht21Channel::humidity);
std::vector<Input<float>> greenhouseAmbientTemperatureInputs{
  groundWestSensor,
  groundEastSensor,
  ceilingWestSensor,
  ceilingEastSensor,
  greenhouseTemp
};
AggregatingProcess<float> greenhouseAmbientTemperature(greenhouseAmbientTemperatureInputs);

// First, the window vents.
Input<float> maxGreenhouseTemp = greenhouseAmbientTemperature.getInputForChannel(AggregationType::maximum);
auto ventsAndDoorsRangeInput = makeRangeConstantInput<float>(15.0, 25.0);
Input<ProcessControlDirection> ventsBangBang = BangBangProcess<float>(maxGreenhouseTemp, ventsAndDoorsRangeInput);
// This is a cooling process, so up is false.
Input<bool> ventCoverInput = DirectionToBooleanProcess(ventsBangBang, false);
Output ventCover = makeCover(
  ROOF_VENT_ACTUATOR_OPEN_PIN,
  ROOF_VENT_ACTUATOR_CLOSE_PIN,
  0,
  HIGH,
  ROOF_VENT_ACTUATION_TIME,
  ventCoverInput,
  runner
);

// Next, the door alarm.
// Pull up when open, so that closed is false.
auto westDoorOpenSensor = DigitalPinInput(DOOR_W_SENSOR_PIN, INPUT_PULLUP);
auto eastDoorOpenSensor = DigitalPinInput(DOOR_E_SENSOR_PIN, INPUT_PULLUP);
auto eitherDoorOpenProcess = OrProcess(westDoorOpenSensor, eastDoorOpenSensor);
Input<std::tuple<std::optional<bool>, std::optional<float>, std::optional<Range<std::optional<float>>>>> doorsDecisionMakingInput = Merging3Process<bool, float, Range<std::optional<float>>>(eitherDoorOpenProcess, maxGreenhouseTemp, ventsAndDoorsRangeInput);
Input<std::string> doorAlarmMessageInput = TranslatingProcess<std::tuple<std::optional<bool>, std::optional<float>, std::optional<Range<std::optional<float>>>>, std::string>(
  doorsDecisionMakingInput,
  [](std::optional<std::tuple<std::optional<bool>, std::optional<float>, std::optional<Range<std::optional<float>>>>> value) {
    if (!value.has_value()) {
      return (std::optional<std::string>)std::nullopt;
    }

    auto combined = value.value();
    // This can't actually be null.
    // At least, not with the current codebase.
    bool doorIsOpen = std::get<0>(combined).value();
    // But this can.
    auto temp = std::get<1>(combined);
    // But if it does, we can't do anything with it.
    if (!temp.has_value()) {
      return (std::optional<std::string>)std::nullopt;
    }
    // This can't be null either.
    auto range = std::get<2>(combined).value();
    auto min = range.min;
    auto max = range.max;

    if (doorIsOpen && min.has_value() && temp.value() < min.value()) {
      std::stringstream message;
      message << "Greenhouse temperature is too cold (" << (min.value() - temp.value()) << " below " << min.value() << ") and at least one door is open. Close it up!";
      return (std::optional<std::string>)message.str();
    }

    if (!doorIsOpen && max.has_value() && temp.value() > max.value()) {
      std::stringstream message;
      message << "Greenhouse temperature is too hot (" << (temp.value() - max.value()) << " above " << max.value() << ") and both doors are closed. Open them up!";
      return (std::optional<std::string>)message.str();
    }

    return (std::optional<std::string>)std::nullopt;
  }
);
Output doorAlarmMessageSender = TwilioMessageOutput(
  doorAlarmMessageInput,
  DOOR_ALARM_MESSAGE_SEND_EVERY,
  twilioConfig,
  runner
);

// Next, the fans for when the vents and doors ain't enough.
// We'll put in a bit of overlap with the vents and doors --
// they'll kick in at 30 degrees and keep working until the temp drops down to 20 again.
auto fanRangeInput = makeRangeConstantInput<float>(20.0, 30.0);
Input<ProcessControlDirection> fanBangBang = BangBangProcess<float>(maxGreenhouseTemp, fanRangeInput);
Input<bool> fanRelayInput = DirectionToBooleanProcess(fanBangBang, false);
Output fanRelay = makeRelay(
  FAN_RELAY_PIN,
  LOW,
  FAN_CYCLE_TIME,
  fanRelayInput,
  runner
);

#define BUTTON_UP_PIN 4
#define BUTTON_RIGHT_PIN 5
#define BUTTON_DOWN_PIN 6
#define BUTTON_LEFT_PIN 7
#define BUTTON_BOUNCE 10

LiquidCrystal lcd(HD44780_RS_PIN, HD44780_EN_PIN, HD44780_D1_PIN, HD44780_D2_PIN, HD44780_D3_PIN, HD44780_D4_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Getting started!");
  Serial.println("Setting up LCD...");
  lcd.begin(16, 2);
  lcd.print("Starting...");

  Serial.println("Setting up input pins...");
  pinMode(DOOR_W_SENSOR_PIN, INPUT_PULLUP);
  pinMode(DOOR_E_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  Serial.println("Buttons and reed switches set up.");
}

void loop() {
}