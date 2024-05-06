#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>

#include <input/Max6675.h>
#include <arduino_test_setup.h>

void test_max6675() {
  Max6675 sensor(SPI_MAX6675_CS_PIN, spiBus());
  auto temp = sensor.read();
  TEST_ASSERT_TRUE(temp.has_value());
  TEST_ASSERT_TRUE(temp.value() > 18 && temp.value() < 25);
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_max6675);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif