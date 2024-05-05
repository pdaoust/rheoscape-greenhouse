#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>

#include <input/Ds18b20.h>

#include <arduino_test_constants.h>

Ds18b20 thermometersBus(ONE_WIRE_DATA_PIN);

void test_ds18b20() {
  for (int i = 0; i < 1000; i ++) {
    delay(1);
    auto readings = thermometersBus.read();
    TEST_ASSERT_EQUAL_MESSAGE(2, readings.size(), "Should have found two thermometers on the bus");
    auto first = readings.begin()->second;
    auto second = readings.end()->second;
    if (readings.begin()->second.has_value() && readings.end()->second.has_value()) {
      // Readings are finally ready.
      TEST_ASSERT_TRUE(first.value() > 18 && first.value() < 25);
      TEST_ASSERT_TRUE(second.value() > 18 && second.value() < 25);
      TEST_PASS();
    }
  }
  TEST_FAIL_MESSAGE("Didn't get a read after ~1000ms");
}

void test_hot_swap_add() {
  Serial.println("Okay, now plug in the third device. You've got five seconds.");
  delay(5000);
  auto readings = thermometersBus.read();
  TEST_ASSERT_EQUAL_MESSAGE(2, readings.size(), "Shouldn't see the new device's reading before scanning");
  thermometersBus.scanBus();
  readings = thermometersBus.read();
  delay(1000);
  readings = thermometersBus.read();
  TEST_ASSERT_EQUAL_MESSAGE(3, readings.size(), "Now it should see the new device's reading");
}

void test_hot_swap_remove() {
  Serial.println("Now take it out. Five more seconds. Go!");
  delay(5000);
  auto readings = thermometersBus.read();
  TEST_ASSERT_EQUAL_MESSAGE(2, readings.size(), "New device's reading should already be gone");
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_ds18b20);
  RUN_TEST(test_hot_swap_add);
  RUN_TEST(test_hot_swap_remove);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}
#endif