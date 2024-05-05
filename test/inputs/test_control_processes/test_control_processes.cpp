#include <unity.h>

#include <Range.h>
#include <input/ControlProcesses.h>

void test_bang_bang_process() {
  StateInput setpoint(Range(19.0f, 21.0f));
  StateInput temp(0.0f);
  BangBangProcess<float> processController(&temp, &setpoint);
  TEST_ASSERT_EQUAL(ProcessControlDirection::up, processController.read());
  temp.write(19.0f);
  TEST_ASSERT_EQUAL(ProcessControlDirection::neutral, processController.read());
  temp.write(21.0f);
  TEST_ASSERT_EQUAL(ProcessControlDirection::neutral, processController.read());
  temp.write(21.1f);
  TEST_ASSERT_EQUAL(ProcessControlDirection::down, processController.read());
  temp.write(20.0f);
  TEST_ASSERT_EQUAL(ProcessControlDirection::neutral, processController.read());
}

void test_direction_to_boolean_process() {
  StateInput process(ProcessControlDirection::neutral);
  DirectionToBooleanProcess heater(&process, true, false);
  DirectionToBooleanProcess cooler(&process, false, false);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  process.write(ProcessControlDirection::up);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  process.write(ProcessControlDirection::neutral);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  process.write(ProcessControlDirection::down);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
  process.write(ProcessControlDirection::neutral);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
  process.write(ProcessControlDirection::up);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  process.write(ProcessControlDirection::down);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
}

void test_whole_shebangbang() {
  StateInput setpoint(Range(19.0f, 21.0f));
  StateInput temp(20.0f);
  BangBangProcess<float> processController(&temp, &setpoint);
  DirectionToBooleanProcess heater(&processController, true, false);
  DirectionToBooleanProcess cooler(&processController, false, false);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  temp.write(18.9f);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  temp.write(19.0f);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  temp.write(21.1f);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
  temp.write(21.0f);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
  temp.write(18.0f);
  TEST_ASSERT_EQUAL(true, heater.read());
  TEST_ASSERT_EQUAL(false, cooler.read());
  temp.write(22.0f);
  TEST_ASSERT_EQUAL(false, heater.read());
  TEST_ASSERT_EQUAL(true, cooler.read());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_bang_bang_process);
  RUN_TEST(test_direction_to_boolean_process);
  RUN_TEST(test_whole_shebangbang);
  UNITY_END();
}