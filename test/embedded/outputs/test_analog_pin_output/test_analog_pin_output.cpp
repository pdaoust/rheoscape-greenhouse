#ifdef PLATFORM_ARDUINO

#include <unity.h>
#include <arduino_test_constants.h>
#include <output/AnalogPinOutput.h>

void test_analog_pin_output() {
  StateInput level(0.0f);
  AnalogPinOutput led((uint8_t)LED_PIN, &level);
  for (uint8_t i = 0; i < 5; i ++) {
    for (float j = 0.0f; j <= 1.0f; j += 0.01f) {
      level.write(j);
      delay(10);
    }
    for (float j = 1.0f; j >= 0.0f; j -= 0.01f) {
      level.write(j);
      led.run();
      delay(10);
    }
  }
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_analog_pin_output);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif