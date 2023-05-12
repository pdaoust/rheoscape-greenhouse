#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal.h>
#include <AsyncTimer.h>
#include <Controller.h>
#include <Process.h>
#include <Range.h>
#include <Sensor.h>

#define SPI_CIPO_PIN 0
#define SPI_PICO_PIN 0
#define I2C_SCL_PIN 16
#define I2C_SDA_PIN 17
#define SHT22_CALIBRATION_OFFSET 0.0
// FIXME: real value please.
#define ONEWIRE_DATA_PIN 0
#define DS18B20_HEATMAT_1_ADDRESS 0x00
#define DS18B20_HEATMAT_1_CALIBRATION_OFFSET 0.0
#define DS18B20_HEATMAT_2_ADDRESS 0x00
#define DS18B20_HEATMAT_2_CALIBRATION_OFFSET 0.0
#define DS18B20_GND_W_ADDRESS 0x00
#define DS18B20_GND_W_CALIBRATION_OFFSET 0.0
#define DS18B20_CEIL_W_ADDRESS 0x00
#define DS18B20_CEIL_W_CALIBRATION_OFFSET 0.0
#define DS18B20_GND_E_ADDRESS 0x00
#define DS18B20_GND_E_CALIBRATION_OFFSET 0.0
#define DS18B20_CEIL_E_ADDRESS 0x00
#define DS18B20_CEIL_E_CALIBRATION_OFFSET 0.0
#define DS18B20_OUTSIDE_ADDRESS 0x00
#define DS18B20_OUTSIDE_CALIBRATION_OFFSET 0.0
// FIXME: real value please.
#define DOOR_W_SENSOR_PIN 0
// FIXME: real value please.
#define DOOR_E_SENSOR_PIN 0
// FIXME: real value please.
#define VENTS_SENSOR_PIN 0

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
// FIXME: real value please.
#define WATER_RELAY_PIN 0
// FIXME: real value please.
#define ROOF_VENT_ACTUATOR_CLOSE_PIN 0
// FIXME: real value please.
#define ROOF_VENT_ACTUATOR_OPEN_PIN 0

class MechanicalRelayProcess : public BangBangProcessControlMethod<float> {
  public:
    MechanicalRelayProcess(TempRange setpointRange, MechanicalRelay controller) : BangBangProcessControlMethod(setpointRange, controller) { }
    MechanicalRelayProcess(TempRange setpointRange, MechanicalRelay controller, float startingInput) : BangBangProcessControlMethod(setpointRange, controller, startingInput) { }
};

class VentActuatorProcess : public BangBangProcessControlMethod<float> {
  public:
    VentActuatorProcess(TempRange setpointRange, LinearActuator controller) : BangBangProcessControlMethod(setpointRange, controller) { }
    VentActuatorProcess(TempRange setpointRange, LinearActuator controller, float startingInput) : BangBangProcessControlMethod(setpointRange, controller, startingInput) { }
};

static MechanicalRelayProcess heatmat1(
  TempRange(26.0, 28.0),
  MechanicalRelay(
    HEATMAT_1_RELAY_PIN,
    LOW
  )
);

static MechanicalRelayProcess heatmat2(
  TempRange(19.0, 21.0),
  MechanicalRelay(
    HEATMAT_2_RELAY_PIN,
    LOW
  )
);

static MechanicalRelayProcess spaceHeater(
  TempRange(-10.0, 0.0),
  MechanicalRelay(
    HEATER_RELAY_PIN,
    LOW,
    FAN_CYCLE_TIME
  )
);

static MechanicalRelayProcess fan(
  TempRange(20.0, 25.0),
  MechanicalRelay(
    FAN_RELAY_PIN,
    // For the fan, you need to turn it off to drive the process variable up.
    // Cuz it cools, right?
    HIGH,
    FAN_CYCLE_TIME
  )
);

static VentActuatorProcess roofVents(
  TempRange(15.0, 25.0),
  LinearActuator(
    // Closing the vents drives the temperature up.
    ROOF_VENT_ACTUATOR_CLOSE_PIN,
    ROOF_VENT_ACTUATOR_OPEN_PIN,
    // Let's go half an hour for the cycle time.
    // The linear actuator and its controller can take a much shorter cycle time,
    // but it's kinda pointless.
    1000 * 60 * 30,
    // I've arbitrarily chosen 'closed' as their starting state.
    // Does this matter?
    // Probably not, because it'll start reading temps soon.
    // Although the setup will delay things somewhat.
    // That said, the cycle time of the thermostats is pretty low anyway.
    PROCESS_DOWN
  )
);

#define BUTTON_UP_PIN 4
#define BUTTON_RIGHT_PIN 5
#define BUTTON_DOWN_PIN 6
#define BUTTON_LEFT_PIN 7
#define BUTTON_BOUNCE 10

#define WIFI_AP "logiehouse2"
#define WIFI_PW "Sistgir*?"

LiquidCrystal lcd(HD44780_RS_PIN, HD44780_EN_PIN, HD44780_D1_PIN, HD44780_D2_PIN, HD44780_D3_PIN, HD44780_D4_PIN);
SHT21 tempHumidityMeter;
BH1750 lightMeter;
OneWire oneWireBus(ONEWIRE_DATA_PIN);
DallasTemperature tempSensors(&oneWireBus);

AsyncTimer ds18b20Loop(
  SENSOR_INTERVAL,
  0,
);

// We don't need to worry about garbage values for any of these on first run,
// because the thermometer loop triggers immediately.
float heatmat1Temp;
float heatmat2Temp;
float greenhouseTemp;
float greenhouseHumidity;
float greenhouseLightLevel;

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

  Wire.begin(16, 15);
  tempHumidityMeter.begin();
  Serial.print("SHT21 status: ");
  Serial.println(tempHumidityMeter.getStatus());
  lightMeter.begin();
  Serial.println("BH1750 set up.");

  // Set up HTTP server event handlers here.
}

static uint8_t runs = 0;

void loop() {
  // Everything in here must be as non-blocking as poss!
  // Except for first run.
  sensorLoop.run([]() {
    // Read the sensors into the statics.
    greenhouseTemp = tempHumidityMeter.requestTemperature() + SHT22_CALIBRATION_OFFSET;
    greenhouseHumidity = tempHumidityMeter.getHumidity();
    greenhouseLightLevel = lightMeter.readLightLevel();

    if (runs == 0) {
      tempSensors.setWaitForConversion(true);
      tempSensors.requestTemperatures();
      tempSensors.setWaitForConversion(false);
    }
    heatmat1Temp = tempSensors.getTempC(DS18B20_HEATMAT_1_ADDRESS) + DS18B20_HEATMAT_1_CALIBRATION_OFFSET;
    heatmat2Temp = tempSensors.getTempC(DS18B20_HEATMAT_2_ADDRESS) + DS18B20_HEATMAT_2_CALIBRATION_OFFSET;
    // Request temperatures for next run, because it'll take a while.
    tempSensors.requestTemperatures();

    firstRun = false;
  });

  if (runs < 2) {
    return;
  }

  // Set process control states.
  // This must happen outside the sensor loop,
  // because some process control methods need to have their loops run all the time.

  // Poll button states.
  // If the HTTP server needs polling, this is where to do that too.

  // Update the UI, send MQTT and WebSocket messages, trigger alarms.
}