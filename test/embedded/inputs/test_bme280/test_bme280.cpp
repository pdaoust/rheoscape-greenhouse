#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <optional>
#include <unity.h>

#include <input/Bme280.h>

#include <arduino_test_setup.h>

void test_bme280() {
  Bme280 sensor(i2cBus());
  for (int i = 0; i < 1000; i ++) {
    delay(1);
    std::optional<float> temp = sensor.readChannel(Bme280Channel::tempC);
    if (temp.has_value()) {
      std::optional<Bme280Reading> allReadings = sensor.read();
      if (!allReadings.has_value()) {
        TEST_FAIL_MESSAGE("After first successful reading, all subsequent readings should have a value");
      }
      TEST_ASSERT_TRUE(temp.value() > 18 && temp.value() < 25);
      TEST_PASS();
    }
  }
  TEST_FAIL_MESSAGE("Didn't get a read after ~1000ms");
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_bme280);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif