#ifndef RHEOSCAPE_BME280_H
#define RHEOSCAPE_BME280_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>
#include <BME280_DEV.h>

enum Bme280Channel {
  tempC,
  humidity,
  pressureKpa,
  altitudeM
};

struct Bme280Reading {
  float tempC;
  float humidity;
  float pressureKpa;
  float altitudeM;
};

class Bme280 : public MultiInput<Bme280Channel, std::optional<float>>, Input<std::optional<Bme280Reading>> {
  private:
    BME280_DEV _input;
    float _tempOffset;
    float _humOffset;
    float _pressOffset;
    float _altOffset;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadPress;
    std::optional<float> _lastReadHum;
    std::optional<float> _lastReadAlt;
    SPIClass* _spi;

    void _read() {
      float temp, press, hum, alt;
      if (_input.getMeasurements(temp, press, hum, alt)) {
        _lastReadTemp = temp + _tempOffset;
        // Pressure is given in millibars, but we want it in kPa.
        _lastReadPress = press * 10 + _pressOffset;
        _lastReadHum = hum + _humOffset;
        _lastReadAlt = alt + _altOffset;
      }
    }

  public:
    Bme280(uint8_t csPin, float tempOffset = 0.0, float humOffset = 0.0, float pressOffset = 0.0, float altOffset = 0.0)
    :
      _spi(new SPIClass(HSPI)),
      _input(BME280_DEV(csPin, HSPI, *_spi)),
      _tempOffset(tempOffset),
      _humOffset(humOffset),
      _pressOffset(pressOffset),
      _altOffset(altOffset)
    {
      while (!_input.begin(FORCED_MODE, OVERSAMPLING_X1, OVERSAMPLING_X1, OVERSAMPLING_X1, IIR_FILTER_OFF, TIME_STANDBY_1000MS));
    }

    ~Bme280() {
      // Not sure if this is needed, but I suspect the BME280 doesn't clean up the SPI reference it gets.
      delete _spi;
    }

    virtual std::optional<float> readChannel(Bme280Channel channel) {
      _read();
      switch (channel) {
        case Bme280Channel::tempC:
          return _lastReadTemp;
        case Bme280Channel::humidity:
          return _lastReadHum;
        case Bme280Channel::pressureKpa:
          return _lastReadPress;
        case Bme280Channel::altitudeM:
          return _lastReadAlt;
        default:
          return std::nullopt;
      }
    }

    virtual std::optional<Bme280Reading> read() {
      _read();
      if (_lastReadTemp.has_value()) {
        return Bme280Reading {
          _lastReadTemp.value(),
          _lastReadHum.value(),
          _lastReadPress.value(),
          _lastReadAlt.value()
        };
      }
      return std::nullopt;
    }
};

#endif

#endif