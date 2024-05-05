#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <optional>
#include <unity.h>

#include <input/GpioInputs.h>

#include <arduino_test_constants.h>

void test_digital_pin_input() {
  DigitalPinInput input1(BUTTON_PIN, INPUT_PULLDOWN);
  TEST_ASSERT_FALSE(input1.read());
  Serial.println("Now close the circuit on pin 1 for two seconds");
  delay(2000);
  TEST_ASSERT_TRUE(input1.read());
  DigitalPinInput input2(2, INPUT_PULLUP);
  TEST_ASSERT_FALSE(input2.read());
  Serial.println("Now close the circuit on pin 2 for two seconds");
  delay(2000);
  TEST_ASSERT_TRUE(input2.read());
  Serial.println("Okay, you're done!");
}

void test_analog_pin_input() {
  AnalogPinInput input(POTENTIOMETER_PIN);
  Serial.println("Now turn the pot down to zero");
  delay(2000);
  TEST_ASSERT_TRUE(input.read() < 0.25);
  Serial.println("Now turn it all the way up");
  delay(2000);
  TEST_ASSERT_TRUE(input.read() > 0.75);
  Serial.println("Now turn it halfway");
  delay(2000);
  TEST_ASSERT_TRUE(input.read() > 0.25 && input.read() < 0.75);
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_digital_pin_input);
  RUN_TEST(test_analog_pin_input);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}
#endif