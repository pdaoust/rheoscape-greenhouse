#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>

#include <helpers/string_format.h>
#include <input/Bh1750.h>
#include <arduino_test_setup.h>

void test_bh1750() {
  Bh1750 lightMeter(1000, i2cBus());
  float lightLevel = lightMeter.read();
  TEST_ASSERT_TRUE(lightLevel > 0);
  float lightLevel2 = lightMeter.read();
  TEST_ASSERT_EQUAL_MESSAGE(lightLevel, lightLevel2, "Bh1750 shouldn't change readings more often than sample interval");
  delay(1000);
  float lightLevel3 = lightMeter.read();
  TEST_ASSERT_NOT_EQUAL_MESSAGE(lightLevel, lightLevel3, string_format("Readings should be different; reading stayed consistent at %0.4f", lightLevel3).c_str());
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_bh1750);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif