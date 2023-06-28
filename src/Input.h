#ifndef RHEOSCAPE_INPUT_H
#define RHEOSCAPE_INPUT_H

#include <Arduino.h>
#include <Timer.h>
#include <map>
#include <optional>
#include <variant>
#include <vector>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SHT2x.h>
#include <BME280_DEV.h>
#include <Wire.h>
#include <BH1750.h>

#include <Range.h>

template <typename TVal>
class Input {
  public:
    virtual std::optional<TVal> read() { return std::nullopt; }
};

template <typename TChan, typename TVal>
class MultiInput {
  public:
    virtual std::optional<TVal> readChannel(TChan channel) { return std::nullopt; }

    Input<TVal> getInputForChannel(TChan channel);
};

template <typename TChan, typename TVal>
class SingleChannelOfMultiInput : public Input<TVal> {
  private:
    MultiInput<TChan, TVal>& _wrappedInput;
    TChan _channel;

  public:
    SingleChannelOfMultiInput(MultiInput<TChan, TVal>& wrappedInput, TChan channel)
      :
        _wrappedInput(wrappedInput),
        _channel(channel)
      { }

    std::optional<TVal> read() {
      return _wrappedInput.readChannel(_channel);
    }
};

template <typename TChan, typename TVal>
Input<TVal> MultiInput<TChan, TVal>::getInputForChannel(TChan channel) {
  return SingleChannelOfMultiInput<TChan, TVal>(*this, channel);
}

// Lift a constant into an input.
template <typename TVal>
class ConstantInput : public Input<TVal> {
  private:
    std::optional<TVal> _value;

  public:
    ConstantInput(std::optional<TVal> value) : _value(value) { }

    std::optional<TVal> read() {
      return _value;
    }
};

// Special case that gets used a lot.
template <typename TVal>
Input<Range<std::optional<TVal>>> makeRangeConstantInput(TVal min, TVal max) {
  return ConstantInput<Range<std::optional<TVal>>>(Range<std::optional<TVal>>{ min, max });
}

// A simple input that just returns whatever the value currently is at the pointer passed to it.
template <typename TVal>
class PointerInput : public Input<TVal> {
  private:
    TVal* _pointer;

  public:
    PointerInput(TVal* pointer) : _pointer(pointer) { }

    std::optional<TVal> read() {
      return &_pointer;
    }
};

// A simple input whose value can be set.
template <typename TVal>
class StateInput : public Input<TVal> {
  private:
    std::optional<TVal> _value;

  public:
    std::optional<TVal> read() {
      return _value;
    }

    void write(std::optional<TVal> value) {
      _value = value;
    }
};

// DeviceAddress is a stupid type to use -- it's just an array so you hvae to pass a pointer.
// And you can't use it as a map key.
// Ints are easier to initialise, and they pass around better.
// But you need a bit of conversion.
void intToDeviceAddress(uint64_t address, DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i ++) {
    deviceAddress[i] = address & 0xFF;
    address >>= 8;
  }
}

uint64_t deviceAddressToInt(DeviceAddress deviceAddress) {
  uint64_t address = 0;
  for (uint8_t i = 0; i < 8; i ++) {
    address += deviceAddress[i] & (0xFF << i * 8);
  }
  return address;
}

class Ds18b20 : public MultiInput<uint64_t, float> {
  private:
    DallasTemperature _inputs;
    std::vector<uint64_t> _deviceAddresses;
    std::map<uint64_t, std::optional<float>> _deviceTemperatures;
    std::map<uint64_t, float> _deviceTemperatureOffsets;
    RepeatTimer _timer;

  public:
    enum Resolution {
      half_degree = 9,
      quarter_degree = 10,
      eighth_degree = 11,
      sixteenth_degree = 12
    };

    Ds18b20(uint8_t dataPin, Resolution resolution = half_degree, std::map<uint64_t, float> deviceTemperatureOffsets = std::map<uint64_t, float>())
    :
      _deviceTemperatureOffsets(deviceTemperatureOffsets),
      // We use an RepeatTimer here because unfortunately you can't poll using DallasTemperature.
      _timer(RepeatTimer(
        millis(),
        // This gnarly math is copied from https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/WaitForConversion/WaitForConversion.ino#L58
        // in which the proper timeout is determined by bit math
        750 / (1 << (12 - resolution)),
        [this, resolution]() {
          for (auto const& pair : _deviceTemperatureOffsets) {
            uint64_t owAddress = pair.first;
            DeviceAddress dAddress;
            intToDeviceAddress(owAddress, dAddress);
            // FIXME: what happens when the device doesn't exist on the bus?
            // Quite likely situation, given my skill cnnecting things together reliably.
            float tempC = _inputs.getTempC(dAddress);
            if (tempC == DEVICE_DISCONNECTED_C) {
              _deviceTemperatures[owAddress] = std::nullopt;
            } else {
              float offset = pair.second;
              _deviceTemperatures[owAddress] = tempC + offset;
            }
          }
          // Set up for next run.
          _inputs.requestTemperatures();
        }
      ))
    {
      // First, find out what devices are on the bus.
      OneWire bus = OneWire(dataPin);
      uint8_t dAddress[8];
      while (bus.search(dAddress)) {
        uint64_t owAddress = deviceAddressToInt(dAddress);
        if (_deviceTemperatureOffsets.find(owAddress) == _deviceTemperatureOffsets.end()) {
          // No temp offset specified, and no awareness of this address.
          // (The the role of the map of offsets is overloaded with keeping track of what devices are on the bus.)
          // Add it to the map with a zero offset.
          _deviceTemperatureOffsets[owAddress] = 0.0;
        }
      }

      _inputs = DallasTemperature(&bus);
      _inputs.setResolution(resolution);
      // Set up in sync mode for first run.
      _inputs.setWaitForConversion(true);
      _inputs.requestTemperatures();
      // Set up in async mode for subsequent runs.
      _inputs.setWaitForConversion(false);
    }

    std::optional<float> readChannel(uint64_t address) {
      _timer.tick();
      return _deviceTemperatures.at(address);
    }
};

enum class Sht21Channel {
  tempC,
  humidity
};

class Sht21 : public MultiInput<Sht21Channel, float> {
  private:
    SHT21 _input;
    float _tempOffset;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadHum;
    // What are we currently polling for?
    // Maps to the underlying library's request type:
    // 0 = no polling happening
    // 1 = polling for temperature
    // 2 = polling for humidity
    uint8_t _mode = 0;

  public:
    Sht21(uint8_t dataPin, uint8_t clockPin, float tempOffset = 0.0)
    :
      _input(SHT21()),
      _tempOffset(tempOffset)
    {
      _input.begin(dataPin, clockPin);
    }

    std::optional<float> readChannel(Sht21Channel channel) {
      switch (_mode) {
        case 0:
          // Not polling for anything. Start the poll on the requested channel.
          switch (channel) {
            case Sht21Channel::tempC:
              _input.requestTemperature();
              _mode = 1;
              // And return the last known value.
              return _lastReadTemp;
            case Sht21Channel::humidity:
              _input.requestHumidity();
              _mode = 2;
              return _lastReadHum;
            // Unreachable; doing it to shut the compiler up.
            default: return std::nullopt;
          }
        case 1:
          // Temp has been requested and we're currently polling for it.
          switch (channel) {
            case Sht21Channel::tempC:
              // Once it's ready and there are no errors,
              // get the temp and reset the polling mode so we know we can poll again.
              if (_input.reqTempReady() && _input.readTemperature()) {
                _lastReadTemp = _input.getTemperature();
                _mode = 0;
              }
              return _lastReadTemp;
            case Sht21Channel::humidity:
              // not currently polling for humidity; just return the last known value.
              return _lastReadHum;
            default: return std::nullopt;
          }
        case 2:
          switch (channel) {
            case Sht21Channel::humidity:
              if (_input.reqHumReady() && _input.readHumidity()) {
                _lastReadHum = _input.getHumidity();
                _mode = 0;
              }
              return _lastReadHum;
            case Sht21Channel::tempC:
              return _lastReadTemp;
            default: return std::nullopt;
          }
        default: return std::nullopt;
      }
    }
};

enum class Bme280Channel {
  tempC,
  humidity,
  pressureKpa
};

class Bme280 : public MultiInput<Bme280Channel, float> {
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

class Bh1750 : public Input<float> {
  private:
    BH1750 _lightMeter;
    RepeatTimer _timer;
    std::optional<float> _lastReadValue;

  public:
    Bh1750(uint8_t address, unsigned long sampleInterval)
    :
      _lightMeter(BH1750(address)),
      _timer(RepeatTimer(
        millis(),
        sampleInterval,
        [this]() {
          if (_lightMeter.measurementReady()) {
            _lastReadValue = _lightMeter.readLightLevel();
          }
          return _lastReadValue;
        }
      ))
    {
      Wire.begin();
      _lightMeter.begin();
    }

    std::optional<float> read() {
      _timer.tick();
      return _lastReadValue;
    }
};

class DigitalPinInput : public Input<bool> {
  private:
    uint8_t _pin;
    bool _onState;
  
  public:
    DigitalPinInput(uint8_t pin, uint8_t mode)
    :
      _pin(pin),
      _onState(mode == INPUT_PULLDOWN)
    {
      pinMode(_pin, mode);
    }

    std::optional<bool> read() {
      bool pinState = digitalRead(_pin);
      return _onState ? pinState : !pinState;
    }
};

class AnalogPinInput : public Input<float> {
  private:
    uint8_t _pin;
    static uint8_t _resolution;
    static bool _resolutionSetOnce;

  public:
    AnalogPinInput(uint8_t pin)
    : _pin(pin)
    {
      if (!_resolutionSetOnce) {
        setResolution(10);
        _resolutionSetOnce = true;
      }
    }

    std::optional<float> read() {
      return (float)analogRead(_pin) / (2 ^ _resolution - 1);
    }

    static void setResolution(uint8_t resolution) {
      _resolution = resolution;
      _resolutionSetOnce = true;
      analogReadResolution(resolution);
    }
};

#endif