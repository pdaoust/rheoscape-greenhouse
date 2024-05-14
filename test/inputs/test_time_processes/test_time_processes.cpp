#include <execinfo.h>
#include <signal.h>
#include <unity.h>

#include <iostream>
#include <helpers/string_format.h>
#include <input/Input.h>
#include <input/TimeProcesses.h>

void test_time_quantising_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  FunctionInput<unsigned long> clocky([](){ return Timekeeper::nowMillis(); });
  TimeQuantisingProcess clockyStepper(&clocky, 10);
  for (unsigned long i = 0; i <= 50; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_EQUAL(floor(i / 10) * 10, clockyStepper.read());
  }
}

void test_throttling_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  FunctionInput<unsigned long> clocky([](){ return Timekeeper::nowMillis(); });
  TimeQuantisingProcess clockyStepper(&clocky, 10);
  TEST_ASSERT_EQUAL(0, clockyStepper.read());
  Timekeeper::setNowSim(5);
  TEST_ASSERT_EQUAL(0, clockyStepper.read());
  Timekeeper::setNowSim(10);
  TEST_ASSERT_EQUAL(10, clockyStepper.read());
  // This is not a quantiser, so it should be able to read intermediate values if it hasn't been triggered in a while.
  Timekeeper::setNowSim(22);
  TEST_ASSERT_EQUAL(22, clockyStepper.read());
}

void test_blinking_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  StateInput switcher(false);
  BlinkingProcess blinker(&switcher, 10, 5);
  TEST_ASSERT_FALSE(blinker.read());
  Timekeeper::tick();
  switcher.write(true);
  TEST_ASSERT_TRUE_MESSAGE(blinker.read(), "should be on the first time");
  Timekeeper::tick(10);
  TEST_ASSERT_FALSE_MESSAGE(blinker.read(), "should be off now");
  Timekeeper::tick(5);
  TEST_ASSERT_TRUE_MESSAGE(blinker.read(), "should be on the second time");
}

void test_slow_pwm_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  StateInput dutyCycle(0.5f);
  SlowPwmProcess pwm(&dutyCycle, 100, 100);
  // It should be on for the first half of 100 ms...
  for (unsigned long i = 0; i < 50; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
  // And it should be off for the second half.
  for (unsigned long i = 50; i < 100; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_FALSE_MESSAGE(pwm.read(), string_format("should be false at %d ms", i).c_str());
  }
  // Double check that the next cycle is the same.
  for (unsigned long i = 100; i < 150; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
  for (unsigned long i = 150; i < 200; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_FALSE_MESSAGE(pwm.read(), string_format("should be false at %d ms", i).c_str());
  }
  // Lastly, it should handle counter rollover gracefully.
  for (unsigned long i = 65500; i < 65550; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
  for (unsigned long i = 65550; i < 65600; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_FALSE_MESSAGE(pwm.read(), string_format("should be false at %d ms", i).c_str());
  }
  for (unsigned long i = 65600; i < 65650; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
  // More than one rollover, for good measure.
  for (unsigned long i = 131000; i < 131050; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
  for (unsigned long i = 131050; i < 131100; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_FALSE_MESSAGE(pwm.read(), string_format("should be false at %d ms", i).c_str());
  }
  for (unsigned long i = 131100; i < 131150; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_TRUE_MESSAGE(pwm.read(), string_format("should be true at %d ms", i).c_str());
  }
}

void test_hysteresis_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  StateInput reading(0);
  HysteresisProcess hysteresiser(&reading, (unsigned long)1, 1);
  TEST_ASSERT_EQUAL(0, hysteresiser.read());
  reading.write(10);
  Timekeeper::tick();
  // Should rise slowly to its target of 10 over 10 milliseconds.
  for (uint8_t i = 1; i < 10; i ++) {
    TEST_ASSERT_EQUAL(i, hysteresiser.read());
    Timekeeper::tick();
  }
  // And if the reading went further up,
  // but went back to its previous value halfway towards it target,
  // the hysteresiser should correct course and follow the reading.
  reading.write(20);
  for (uint8_t i = 10; i < 15; i ++) {
    TEST_ASSERT_EQUAL(i, hysteresiser.read());
    Timekeeper::tick();
  }
  reading.write(10);
  for (uint8_t i = 13; i > 10; i --) {
    TEST_ASSERT_EQUAL(i, hysteresiser.read());
    Timekeeper::tick();
  }
}

void test_exponential_moving_average_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  StateInput sensor(5.0f);
  ExponentialMovingAverageProcess smoother(&sensor, 40);
  TEST_ASSERT_EQUAL_FLOAT(5.0f, smoother.read());
  for (int i = 0; i < 50; i ++) {
    // A sawtooth wave that's 4× the frequency of the smoother's cutoff frequency.
    for (int j = 0; j <= 10; j ++) {
      sensor.write(j);
      Timekeeper::tick();
      float integrated = smoother.read();
      // Shouldn't vary by much more than 1 unit.
      TEST_ASSERT_TRUE(integrated < 5.5f && integrated > 4.5f);
      //std::cout << string_format("At timestamp %d, sensor is %.3f and smoother is %.3f", Timekeeper::nowMillis(), sensor.read(), integrated).c_str();
    }
  }
}

void test_timed_latch_process() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  StateInput sensor(false);
  TimedLatchProcess latch(&sensor, 10);
  TEST_ASSERT_FALSE(latch.read());
  // It should turn on, then off, even if the sensor stays true.
  Timekeeper::tick();
  sensor.write(true);
  TEST_ASSERT_TRUE(latch.read());
  for (uint8_t i = 0; i < 9; i ++) {
    Timekeeper::tick();
    TEST_ASSERT_TRUE(latch.read());
  }
  Timekeeper::tick();
  TEST_ASSERT_FALSE(latch.read());
  // And it should turn on, then hold for the timeout, even if the sensor immediately turns off.
  Timekeeper::tick();
  sensor.write(false);
  TEST_ASSERT_FALSE(latch.read());
  Timekeeper::tick();
  sensor.write(true);
  TEST_ASSERT_TRUE(latch.read());
  for (uint8_t i = 0; i < 9; i ++) {
    Timekeeper::tick();
    TEST_ASSERT_TRUE(latch.read());
  }
  Timekeeper::tick();
  TEST_ASSERT_FALSE(latch.read());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_time_quantising_process);
  RUN_TEST(test_throttling_process);
  RUN_TEST(test_blinking_process);
  RUN_TEST(test_slow_pwm_process);
  RUN_TEST(test_hysteresis_process);
  RUN_TEST(test_exponential_moving_average_process);
  RUN_TEST(test_timed_latch_process);
  UNITY_END();
}