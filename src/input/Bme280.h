#ifndef RHEOSCAPE_BME280_H
#define RHEOSCAPE_BME280_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>
#include <BME280_DEV.h>

enum class Bme280Channel {
  tempC,
  humidity,
  pressureKpa
};

class Bme280 : public MultiInput<Bme280Channel, std::optional<float>> {
  private:
    BME280_DEV _input;
    float _tempOffset;
    float _humOffset;
    float _pressOffset;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadHum;
    std::optional<float> _lastReadPress;
    SPIClass* _spi;

  public:
    Bme280(uint8_t csPin, float tempOffset = 0.0, float humOffset = 0.0, float pressOffset = 0.0)
    :
      _spi(new SPIClass(HSPI)),
      _input(BME280_DEV(csPin, HSPI, *_spi)),
      _tempOffset(tempOffset),
      _humOffset(humOffset),
      _pressOffset(pressOffset)
    {
      while (!_input.begin(FORCED_MODE, OVERSAMPLING_X1, OVERSAMPLING_X1, OVERSAMPLING_X1, IIR_FILTER_OFF, TIME_STANDBY_1000MS));
    }

    ~Bme280() {
      // Not sure if this is needed, but I suspect the BME280 doesn't clean up the SPI reference it gets.
      delete _spi;
    }

    std::optional<float> readChannel(Bme280Channel channel) {
      float temp, press, hum, _;
      if (_input.getMeasurements(temp, press, hum, _)) {
        _lastReadTemp = temp;
        // Pressure is given in millibars, but we want it in kPa.
        _lastReadPress = press * 10;
        _lastReadHum = hum;
      }

      switch (channel) {
        case Bme280Channel::tempC:
          return _lastReadTemp;
        case Bme280Channel::pressureKpa:
          return _lastReadPress;
        case Bme280Channel::humidity:
          return _lastReadHum;
      }
    }
};

#endif

#endif