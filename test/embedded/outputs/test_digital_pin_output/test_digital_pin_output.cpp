#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>
#include <arduino_test_setup.h>
#include <output/DigitalPinOutput.h>

void test_digital_pin_output() {
  StateInput pinState(false);
  DigitalPinOutput led((uint8_t)LED_PIN, HIGH, &pinState);
  for (uint8_t i = 0; i < 100; i ++) {
    pinState.write(true);
    led.run();
    delay(100);
    pinState.write(false);
    led.run();
    delay(100);
  }
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_digital_pin_output);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif