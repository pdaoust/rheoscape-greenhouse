#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>
#include <arduino_test_setup.h>
#include <output/OutputFactories.h>

void test_make_relay() {
  StateInput state(false);
  DigitalPinOutput relay = makeRelay(LED_PIN, LOW, 1000, &state);
  for (int i = 0; i < 100; i ++) {
    state.write(true);
    relay.run();
    delay(110);
    state.write(false);
    relay.run();
    delay(110);
  }
}

void test_make_cover() {
  StateInput state(CoverState::closed);
  MotorDriver cover = makeCover(MOTOR_DRIVER_FWD_PIN, MOTOR_DRIVER_REV_PIN, HIGH, 3000, &state);
  for (int i = 0; i < 100; i ++) {
    state.write(CoverState::open);
    cover.run();
    delay(220);
    state.write(CoverState::closed);
    cover.run();
    delay(220);
  }
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_make_relay);
  RUN_TEST(test_make_cover);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif