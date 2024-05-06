#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>

#include <input/Sht21.h>
#include <arduino_test_setup.h>

void test_sht21() {
  Sht21 sensor(i2cBus());
  sensor.readChannel(Sht21Channel::tempC);
  delay(100);
  auto temp = sensor.readChannel(Sht21Channel::tempC);
  TEST_ASSERT_TRUE(temp.has_value());
  TEST_ASSERT_TRUE(temp.value() > 18 && temp.value() < 25);
  sensor.readChannel(Sht21Channel::humidity);
  delay(100);
  auto hum = sensor.readChannel(Sht21Channel::humidity);
  TEST_ASSERT_TRUE(hum.has_value());
  TEST_ASSERT_TRUE(hum.value() > 0 && hum.value() <= 100);
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_sht21);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif