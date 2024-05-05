#ifdef PLATFORM_ARDUINO

#include <Arduino.h>
#include <unity.h>
#include <arduino_test_constants.h>
#include <output/MotorDriver.h>

void test_motor_driver() {
  StateInput motorDirection(0.0f);
  MotorDriver smoothMotor(MOTOR_DRIVER_FWD_PIN, MOTOR_DRIVER_REV_PIN, MOTOR_DRIVER_PWM_PIN, HIGH, &motorDirection);
  for (uint8_t i = 0; i < 10; i ++) {
    for (float j = 0.0f; j <= 1.0f; j += 0.01f) {
      motorDirection.write(j);
      smoothMotor.run();
      delay(10);
    }
    for (float j = 1.0f; j >= -1.0f; j -= 0.01f) {
      motorDirection.write(j);
      smoothMotor.run();
      delay(10);
    }
    for (float j = -1.0f; j <= 0.0f; j += 0.01f) {
      motorDirection.write(j);
      smoothMotor.run();
      delay(10);
    }
  }
  delay(1000);
  MotorDriver notSmoothMotor(MOTOR_DRIVER_FWD_PIN, MOTOR_DRIVER_REV_PIN, 0, HIGH, &motorDirection);
  for (uint8_t i = 0; i < 10; i ++) {
    motorDirection.write(1.0f);
    notSmoothMotor.run();
    delay(1000);
    motorDirection.write(0.0f);
    notSmoothMotor.run();
    delay(1000);
    motorDirection.write(-1.0f);
    notSmoothMotor.run();
    delay(1000);
    motorDirection.write(0.0f);
    notSmoothMotor.run();
    delay(1000);
  }
}

void setup() {
  // Apparently this is important for testing.
  // The documentation calls it a 'service delay'.
  delay(2000);
  UNITY_BEGIN();
  RUN_TEST(test_motor_driver);
  UNITY_END();
}

void loop() {}

#else

int main(int argc, char **argv) {}

#endif