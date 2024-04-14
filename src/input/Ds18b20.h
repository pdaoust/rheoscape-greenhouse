#ifndef RHEOSCAPE_DS18B20_H
#define RHEOSCAPE_DS18B20_H

#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <input/Input.h>
#include <OneWire.h>
#include <DallasTemperature.h>

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

class Ds18b20 : public MultiInput<uint64_t, std::optional<float>> {
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
        Timekeeper::nowMillis(),
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

#endif

#endif