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

void initI2cBus() {
  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin();
}

void initSpiBus() {
  SPI.begin(SPI_CLK_PIN, SPI_CIPO_PIN, SPI_COPI_PIN);
}

#endif