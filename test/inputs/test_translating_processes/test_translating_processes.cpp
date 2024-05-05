#include <unity.h>

#include <input/TranslatingProcesses.h>

void test_translating_process() {
  StateInput integerInput(3);
  TranslatingProcess<int, float> halver(&integerInput, [](int num) { return (float)num / 2.0f; });
  TEST_ASSERT_EQUAL(1.5f, halver.read());
  integerInput.write(5);
  TEST_ASSERT_EQUAL(2.5f, halver.read());
}

void test_translating_optional_process() {
  StateInput maybeIntegerInput((std::optional<int>)3);
  TranslatingOptionalProcess<int, float> halver(&maybeIntegerInput, [](int num) { return (float)num / 2.0f; });
  TEST_ASSERT_TRUE(halver.read().has_value());
  TEST_ASSERT_EQUAL(1.5f, halver.read().value());
  maybeIntegerInput.write(std::nullopt);
  TEST_ASSERT_FALSE(halver.read().has_value());
}

void test_calibration_process() {
  StateInput input(2.4f);
  ConstantInput offset(0.8f);
  CalibrationProcess calibrated(&input, &offset);
  TEST_ASSERT_EQUAL_FLOAT(3.2f, calibrated.read());
  input.write(3.1f);
  TEST_ASSERT_EQUAL_FLOAT(3.9f, calibrated.read());
}

void test_calibration_optional_process() {
  StateInput input((std::optional<float>)2.4f);
  ConstantInput offset(0.8f);
  CalibrationOptionalProcess calibrated(&input, &offset);
  TEST_ASSERT_TRUE(calibrated.read().has_value());
  TEST_ASSERT_EQUAL_FLOAT(3.2f, calibrated.read().value());
  input.write(std::nullopt);
  TEST_ASSERT_FALSE(calibrated.read().has_value());
}

void test_calibration_multi_process() {
  std::map<int, Input<float>*> channels = {
    { 0, new ConstantInput(2.4f) },
    { 1, new ConstantInput(3.1f) },
    { 2, new ConstantInput(-15.2f) }
  };
  MapMultiInput inputs(&channels);
  std::map<int, Input<float>*> offsets = {
    { 0, new ConstantInput(0.1f) },
    { 1, new ConstantInput(-0.8f) },
    { 2, new ConstantInput(3.6f) }
  };
  MapMultiInput adjustments(&offsets);
  CalibrationMultiProcess calibrated(&inputs, &adjustments);
  TEST_ASSERT_EQUAL_FLOAT(2.5f, calibrated.readChannel(0));
  TEST_ASSERT_EQUAL_FLOAT(2.3f, calibrated.readChannel(1));
  TEST_ASSERT_EQUAL_FLOAT(-11.6f, calibrated.readChannel(2));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_translating_process);
  RUN_TEST(test_translating_optional_process);
  RUN_TEST(test_calibration_process);
  RUN_TEST(test_calibration_optional_process);
  RUN_TEST(test_calibration_multi_process);
  UNITY_END();
}