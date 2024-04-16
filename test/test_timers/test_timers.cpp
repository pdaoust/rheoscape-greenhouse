#include <memory>

#include <unity.h>

#include <Runnable.h>
#include <Timer.h>

void test_timekeeper() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  for (uint16_t i = 0; i <= 100; i ++) {
    Timekeeper::setNowSim(i);
    TEST_ASSERT_EQUAL(i, Timekeeper::nowMillis());
  }
  for (uint16_t i = 1; i < 100; i ++) {
    Timekeeper::tick();
    TEST_ASSERT_EQUAL(i + 100, Timekeeper::nowMillis());
  }
}

void test_timer_runs() {
  bool didRun = false;
  Timekeeper::setSource(TimekeeperSource::simTime);
  Timer timer(100, [&didRun]() {
    didRun = true;
  });
  for (uint16_t i = 0; i < 100; i ++) {
    Timekeeper::setNowSim(i);
    timer.run();
    TEST_ASSERT_FALSE(didRun);
  }
  Timekeeper::setNowSim(100);
  timer.run();
  TEST_ASSERT_TRUE(didRun);
}

void test_timer_knows_its_state() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  Timer timer(100, [](){});
  for (uint16_t i = 1; i < 100; i ++) {
    Timekeeper::setNowSim(i);
    timer.run();
    TEST_ASSERT_FALSE(timer.isComplete());
    TEST_ASSERT_FALSE(timer.isCancelled());
    TEST_ASSERT_TRUE(timer.isRunning());
  }
  Timekeeper::setNowSim(100);
  timer.run();
  TEST_ASSERT_TRUE(timer.isComplete());
  TEST_ASSERT_FALSE(timer.isCancelled());
  TEST_ASSERT_FALSE(timer.isRunning());
  // It shouldn't think it's been cancelled if the cancelling happened after it completed.
  timer.cancel();
  TEST_ASSERT_TRUE(timer.isComplete());
  TEST_ASSERT_FALSE(timer.isCancelled());
  TEST_ASSERT_FALSE(timer.isRunning());
  // But if a timer has been cancelled before completion, it should know that.
  timer.restart();
  for (uint8_t i = 0; i <= 100; i ++) {
    Timekeeper::tick();
    if (i == 50) {
      timer.cancel();
      TEST_ASSERT_TRUE(timer.isCancelled());
      TEST_ASSERT_FALSE(timer.isRunning());
    }
  }
  TEST_ASSERT_FALSE(timer.isComplete());
}

void test_timer_can_be_cancelled() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  Timer timer(100, [](){});
  timer.run();
  timer.cancel();
  for (uint16_t i = 1; i <= 100; i ++) {
    Timekeeper::setNowSim(i);
    timer.run();
    TEST_ASSERT_FALSE(timer.isRunning());
  }
}

void test_timer_can_repeat() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  uint8_t didRunTimes = 0;
  Timer timer(10, [&didRunTimes](){ didRunTimes ++; }, 3);
  // Run the clock 20 ms past the last expected run.
  for (uint16_t i = 1; i <= 50; i ++) {
    Timekeeper::setNowSim(i);
    timer.run();
  }
  TEST_ASSERT_EQUAL(3, didRunTimes);
  TEST_ASSERT_TRUE(timer.isComplete());
  TEST_ASSERT_FALSE(timer.isRunning());
}

void test_timer_runs_if_interval_is_past() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  uint8_t didRunTimes = 0;
  Timer timer(10, [&didRunTimes](){ didRunTimes ++; }, 3);
  // Before the interval is up, it shouldn't have run at all.
  timer.run();
  TEST_ASSERT_EQUAL(0, didRunTimes);
  // Pretend an expensive process hogged up one interval + 1 ms.
  Timekeeper::setNowSim(11);
  timer.run();
  TEST_ASSERT_EQUAL(1, didRunTimes);
  // Now pretend it hogged up two more intervals + 1 ms.
  Timekeeper::setNowSim(31);
  timer.run();
  TEST_ASSERT_EQUAL(3, didRunTimes);
}

// A timer should handle the dreaded millis rollover without complaint.
void test_timer_rolls_over() {
  bool didRun = false;
  Timekeeper::setSource(TimekeeperSource::simTime);
  Timekeeper::setNowSim(ULONG_MAX - 5);
  Timer timer(100, [&didRun]() {
    didRun = true;
  });
  for (uint16_t i = 0; i < 100; i ++) {
    timer.run();
    TEST_ASSERT_FALSE(didRun);
    Timekeeper::tick();
  }
  timer.run();
  TEST_ASSERT_TRUE(didRun);
}

void test_timer_can_run_immediately() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  bool didRun = false;
  Timer timer(10, [&didRun]() { didRun = true; }, 2, true);
  TEST_ASSERT_TRUE(didRun);
}

void test_one_shot_timer_cannot_run_immediately() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  try {
    Timer timer(10, [](){}, 1, true);
    TEST_FAIL_MESSAGE("didn't throw an exception");
  } catch (std::invalid_argument e) {
    TEST_PASS();
  }
}

void test_throttle_throttles() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  uint8_t didRunTimes = 0;
  Throttle throttle(10, [&didRunTimes]() { didRunTimes ++; });
  for (uint8_t i = 1; i < 10; i ++) {
    TEST_ASSERT_TRUE(throttle.tryRun());
    TEST_ASSERT_EQUAL(i, didRunTimes);
    for (uint8_t j = 0; j < 9; j ++) {
      Timekeeper::tick();
      TEST_ASSERT_FALSE(throttle.tryRun());
      TEST_ASSERT_EQUAL(i, didRunTimes);
    }
    Timekeeper::tick();
  }
}

void test_throttle_can_clear() {
  Timekeeper::setSource(TimekeeperSource::simTime);
  uint8_t didRunTimes = 0;
  Throttle throttle(10, [&didRunTimes]() { didRunTimes ++; });
  throttle.tryRun();
  TEST_ASSERT_EQUAL(1, didRunTimes);
  throttle.tryRun();
  TEST_ASSERT_EQUAL(1, didRunTimes);
  throttle.clear();
  throttle.tryRun();
  TEST_ASSERT_EQUAL(2, didRunTimes);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_timekeeper);
  RUN_TEST(test_timer_runs);
  RUN_TEST(test_timer_knows_its_state);
  RUN_TEST(test_timer_can_be_cancelled);
  RUN_TEST(test_timer_can_repeat);
  RUN_TEST(test_timer_runs_if_interval_is_past);
  RUN_TEST(test_timer_rolls_over);
  RUN_TEST(test_timer_can_run_immediately);
  RUN_TEST(test_throttle_throttles);
  RUN_TEST(test_throttle_can_clear);
  UNITY_END();
}