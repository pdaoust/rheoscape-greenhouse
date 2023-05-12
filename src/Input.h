#ifndef PAULDAOUST_INPUT_H
#define PAULDAOUST_INPUT_H

#include <Arduino.h>
#include <AsyncTimer.h>
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

template <typename TVal>
class Input {
  public:
    virtual std::optional<TVal> read();
};

template <typename TChan, typename TVal>
class MultiInput {
  public:
    virtual std::optional<TVal> readChannel(TChan channel);
    Input<TVal> getInputForChannel(TChan channel);
};

template <typename TChan, typename TVal>
class SingleChannelOfMultiInput : public Input<TVal> {
  private:
    MultiInput<TChan, TVal> _wrappedInput;
    TChan _channel;

  public:
    SingleChannelOfMultiInput(MultiInput<TChan, TVal> wrappedInput, TChan channel)
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
  return SingleChannelOfMultiInput(this, channel);
}

template <typename TVal>
class PointerInput : Input<TVal> {
  private:
    TVal* _pointer;

  public:
    PointerInput(TVal* pointer) : _pointer(pointer) { }

    std::optional<TVal> read() {
      return &_pointer;
    }
};

template <typename TVal>
class SettableInput : Input<TVal> {
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

enum AggregationType {
  average,
  maximum,
  minimum
};

template <typename TVal>
class InputAggregator : MultiInput<AggregationType, TVal> {
  private:
    std::vector<Input<TVal>> _inputs;
  
  public:
    InputAggregator(std::vector<Input<TVal>> inputs)
    : _inputs(inputs)
    { }

    std::optional<TVal> readChannel(AggregationType channel) {
      std::optional<TVal> acc;
      switch (channel) {
        case average:
          uint count;
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              count ++;
              acc += value;
            }
          }
          acc = acc.has_value() ? acc / count : acc;
          break;
        case minimum:
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              acc = acc.has_value() ? min(acc, value) : value;
            }
          }
          break;
        case maximum:
          for (uint i = 0; i < _inputs.size(); i ++) {
            std::optional<TVal> value = _inputs[i].read();
            if (value.has_value()) {
              acc = acc.has_value() ? max(acc, value) : value;
            }
          }
          break;
      }

      return acc;
    }
};

template <typename TVal>
class ThrottledInput : Input<TVal>, Runnable {
  private:
    Input<TVal> _wrappedInput;
    ThrottlingTimer _timer;
    std::optional<TVal> _lastReadValue;
  
  public:
    ThrottledInput(Input<TVal> wrappedInput, unsigned long interval)
    :
      _wrappedInput(wrappedInput),
      _timer(ThrottlingTimer(
        interval,
        [this]() {
          _lastReadValue = _wrappedInput.read();
          _timer.reset();
        }
      ))
    { }

    std::optional<TVal> read() {
      run();
      return _lastReadValue;
    }

    void run() {
      _timer.run();
    }
};

template <typename TOuter, typename TInner>
class InputTranslator : Input<TOuter> {
  private:
    Input<TInner> _wrappedInput;
    const std::function<std::optional<TOuter>(std::optional<TInner>)>& _translator;

  public:
    InputTranslater(Input<Tinner> wrappedInput, const std::function<std::optional<TOuter>(std::optional<TInner>)>& translator)
    :
      _wrappedInput(wrappedInput),
      _translator(translator);
    { }

    st::optional<TOuter> read() {
      return _translator(_wrappedInput.read());
    }
};

// Add a delay into a input's reading,
// where it takes a given amount of time to reach the new reading.
// For example, for a hysteresiser that can only move 1 step per second,
// if the underlying input read 10 a second ago and now reads 20,
// it will actually read 11 now, 12 next second, and so forth
// until it finally reads 20 in 10 seconds.
// Of course, if the input goes back down to 10 the next second,
// it'll head towards that value.
template <typename TVal>
class InputHysteresiser : Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    unsigned long _interval;
    TVal _stepsPerInterval;
    std::optional<TVal> _lastValue;
    unsigned long _lastRun;

  public:
    InputHysteresiser(Input<TVal> wrappedInput, unsigned long interval, TVal stepsPerInterval)
    :
      _wrappedInput(wrappedInput),
      _interval(interval),
      _stepsPerInterval(stepsPerInterval)
    { }
  
    std::optional<TVal> read() {
      std::optional<TVal> newValue = _wrappedInput.read();
      if (!newValue.has_value()) {
        return _lastValue;
      }

      unsigned long now = millis();
      if (_lastRun) {
        _lastValue = _lastValue + (float)(now - _lastRun) / _interval * _stepsPerInterval;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
    }
};

// Smooth a input reading over a moving average time interval in milliseconds,
// using the exponential moving average or single-pole IIR method.
template <typename TVal>
class InputExponentialMovingAverager : Input<TVal> {
  private:
    Input<TVal> _wrappedInput;
    unsigned long _averageOver;
    std::optional<TVal> _lastValue;
    unsigned long _lastRun;
  
  public:
    InputExponentialMovingAverager(Input<TVal> wrappedInput, unsigned long averageOver)
    :
      _wrappedInput(wrappedInput),
      _averageOver(averageOver)
    { }

    std::optional<TVal> read() {
      std::optional<TVal> newValue = _wrappedInput.read();
      if (!newValue.has_value()) {
        return _lastValue;
      }

      unsigned long now = millis();
      if (_lastRun) {
        float intervalsSinceLastRun = (float)(now - _lastRun) / _averageOver;
        _lastValue -= _lastValue / intervalsSinceLastRun;
        _lastValue += newValue / intervalsSinceLastRun;
      } else {
        _lastValue = newValue;
      }

      _lastRun = now;
      return _lastValue;
    }
};

typedef std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> OneWireAddress;

class Ds18b20 : MultiInput<const OneWireAddress, float> {
  private:
    DallasTemperature _inputs;
    std::vector<OneWireAddress> _deviceAddresses;
    std::map<OneWireAddress, float> _deviceTemperatures;
    std::map<OneWireAddress, float> _deviceTemperatureOffsets;
    AsyncTimer _timer;
    bool _firstRun;

  public:
    enum Resolution {
      half_degree = 9,
      quarter_degree = 10,
      eighth_degree = 11,
      sixteenth_degree = 12
    };

    Ds18b20(uint8_t dataPin, std::vector<OneWireAddress> addresses, Resolution resolution = half_degree, std::map<OneWireAddress, float> deviceTemperatureOffsets = std::map<OneWireAddress, float>())
    :
      _deviceAddresses(addresses),
      _deviceTemperatureOffsets(deviceTemperatureOffsets),
      // We use an AsyncTimer here because unfortunately you can't poll using DallasTemperature.
      _timer(AsyncTimer(
        // This gnarly math is copied from https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/WaitForConversion/WaitForConversion.ino#L58
        // in which the proper timeout is determined by bit math
        750 / (1 << (12 - resolution)),
        0,
        [this, resolution]() {
          if (_firstRun) {
            _inputs.setResolution(resolution);
            // Set up in sync mode for first run.
            _inputs.setWaitForConversion(true);
            _inputs.requestTemperatures();
            // Set up in async mode for subsequent runs.
            _inputs.setWaitForConversion(false);
          }
          for (const OneWireAddress &address : _deviceAddresses) {
            auto maybeTempOffsetPair = _deviceTemperatureOffsets.find(address);
            float tempOffset = (maybeTempOffsetPair == _deviceTemperatureOffsets.end())
              ? 0.0
              : maybeTempOffsetPair->second;
            DeviceAddress addressA {
              std::get<0>(address),
              std::get<1>(address),
              std::get<2>(address),
              std::get<3>(address),
              std::get<4>(address),
              std::get<5>(address),
              std::get<6>(address),
              std::get<7>(address)
            };
            _deviceTemperatures[address] = _inputs.getTempC(addressA) + tempOffset;
          }
          // Set up for next run.
          _inputs.requestTemperatures();
        }
      ))
    {
      auto oneWire = OneWire(dataPin);
      _inputs = DallasTemperature(&oneWire);
    }

    std::optional<float> readChannel(const OneWireAddress address) {
      _timer.run();
      return _deviceTemperatures.at(address);
    }
};

enum class Sht21Channel {
  tempC,
  humidity
};

class Sht21 : MultiInput<Sht21Channel, float> {
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
          }
      }
    }
};

enum class Bme280Channel {
  tempC,
  humidity,
  pressureKpa
};

class Bme280 : MultiInput<Bme280Channel, float> {
  private:
    BME280_DEV _input;
    float _tempOffset;
    float _humOffset;
    float _pressOffset;
    std::optional<float> _lastReadTemp;
    std::optional<float> _lastReadHum;
    std::optional<float> _lastReadPress;

    SPIClass& _instantiateSpiClassRef() {
      SPIClass spi(HSPI);
      return spi;
    } 

  public:
    Bme280(uint8_t csPin, float tempOffset = 0.0, float humOffset = 0.0, float pressOffset = 0.0)
    :
      _input(BME280_DEV(csPin, HSPI, _instantiateSpiClassRef())),
      _tempOffset(tempOffset),
      _humOffset(humOffset),
      _pressOffset(pressOffset)
    {
      while (!_input.begin(FORCED_MODE, OVERSAMPLING_X1, OVERSAMPLING_X1, OVERSAMPLING_X1, IIR_FILTER_OFF, TIME_STANDBY_1000MS));
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

class Bh1750 : Input<float> {
  private:
    BH1750 _lightMeter;
    AsyncTimer _sampleInterval;
    std::optional<float> _lastReadValue;

  public:
    Bh1750(uint8_t address, unsigned long sampleInterval)
    :
      _lightMeter(BH1750(address)),
      _sampleInterval(AsyncTimer(
        sampleInterval,
        0,
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
      return _lastReadValue;
    }
};

class OnePinInput : Input<bool> {
  private:
    uint8_t _pin;
    bool _onState;
    unsigned long _debounceInterval;
  
  public:
    OnePinInput(uint8_t pin, uint8_t mode, unsigned long debounceInterval = 0)
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

#endif