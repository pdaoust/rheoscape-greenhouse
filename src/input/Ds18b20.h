#ifndef RHEOSCAPE_DS18B20_H
#define RHEOSCAPE_DS18B20_H

#ifdef PLATFORM_ARDUINO

#include <deque>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <Timer.h>
#include <input/Input.h>

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

class Ds18b20 : public MultiInput<uint64_t, std::optional<float>>, public Input<std::map<uint64_t, std::optional<float>>> {
  private:
    OneWire* _bus;
    DallasTemperature _inputs;
    std::vector<uint64_t> _deviceAddresses;
    std::map<uint64_t, std::optional<float>> _deviceTemperatures;
    Timer _timer;

  public:
    enum Resolution {
      half_degree = 9,
      quarter_degree = 10,
      eighth_degree = 11,
      sixteenth_degree = 12
    };

    Ds18b20(OneWire* bus, Resolution resolution = half_degree)
    :
      _bus(bus),
      // We use a Timer to throttle reads here because unfortunately you can't poll using DallasTemperature.
      _timer(Timer(
        // This gnarly math is copied from https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/WaitForConversion/WaitForConversion.ino#L58
        // in which the proper timeout is determined by bit math
        750 / (1 << (12 - resolution)),
        [this, resolution]() {
          _deviceTemperatures = std::map<uint64_t, std::optional<float>>();
          for (int i = 0; i < _deviceAddresses.size(); i ++) {
            uint64_t owAddress = _deviceAddresses[i];
            DeviceAddress dAddress;
            intToDeviceAddress(owAddress, dAddress);
            float tempC = _inputs.getTempC(dAddress);
            if (tempC == DEVICE_DISCONNECTED_C) {
              _deviceTemperatures[owAddress] = std::nullopt;
            } else {
              _deviceTemperatures[owAddress] = tempC;
            }
          }
          // Set up for next run.
          _inputs.requestTemperatures();
        },
        std::nullopt,
        true
      ))
    {
      // First, find out what devices are on the bus.
      scanBus();

      _inputs = DallasTemperature(_bus);
      _inputs.setResolution(resolution);
      // Set up in async mode.
      _inputs.setWaitForConversion(false);
    }

    virtual std::optional<float> readChannel(uint64_t address) {
      _timer.run();
      if (_deviceTemperatures.find(address) != _deviceTemperatures.end()) {
        return _deviceTemperatures[address];
      }
      return std::nullopt;
    }

    virtual std::map<uint64_t, std::optional<float>> read() {
      _timer.run();
      return _deviceTemperatures;
    }

    void scanBus() {
      uint8_t dAddress[8];
      _deviceAddresses = std::vector<uint64_t>();
      while (_bus->search(dAddress)) {
        _deviceAddresses.push_back(deviceAddressToInt(dAddress));
      }
    }

    void addChannel(uint8_t channel) {
      _deviceAddresses.push_back(channel);
    }
};

#endif

#endif