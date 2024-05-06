#ifdef PLATFORM_ARDUINO

#define ONE_WIRE_DATA_PIN 0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 0
#define SPI_COPI_PIN 0
#define SPI_CIPO_PIN 0
#define SPI_CLK_PIN 0
#define SPI_BME280_CS_PIN 0
#define SPI_MAX6675_CS_PIN 0
#define BUTTON_PIN 0
#define POTENTIOMETER_PIN 0
#define LED_PIN 0
#define MOTOR_DRIVER_FWD_PIN 0
#define MOTOR_DRIVER_REV_PIN 0
#define MOTOR_DRIVER_PWM_PIN 0

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <OneWire.h>

TwoWire* i2cBus() {
  static bool i2cBusIsInitialised = false;
  if (!i2cBusIsInitialised) {
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    i2cBusIsInitialised = true;
  }
  return &Wire;
}

SPIClass* spiBus() {
  static bool spiBusIsInitialised = false;
  if (!spiBusIsInitialised) {
    SPI.begin(SPI_CLK_PIN, SPI_CIPO_PIN, SPI_COPI_PIN);
    spiBusIsInitialised = true;
  }
  return &SPI;
}

OneWire* oneWireBus() {
  static OneWire* bus;
  static bool oneWireBusIsInitialised;
  if (!oneWireBusIsInitialised) {
    bus = new OneWire(ONE_WIRE_DATA_PIN);
    oneWireBusIsInitialised = true;
  }
  return bus;
}

#endif