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

void test_one_point_calibration_process() {
  StateInput input(2.4f);
  ConstantInput offset(0.8f);
  OnePointCalibrationProcess calibrated(&input, &offset);
  TEST_ASSERT_EQUAL_FLOAT(3.2f, calibrated.read());
  input.write(3.1f);
  TEST_ASSERT_EQUAL_FLOAT(3.9f, calibrated.read());
}

void test_one_point_calibration_optional_process() {
  StateInput input((std::optional<float>)2.4f);
  ConstantInput offset(0.8f);
  OnePointCalibrationOptionalProcess calibrated(&input, &offset);
  TEST_ASSERT_TRUE(calibrated.read().has_value());
  TEST_ASSERT_EQUAL_FLOAT(3.2f, calibrated.read().value());
  input.write(std::nullopt);
  TEST_ASSERT_FALSE(calibrated.read().has_value());
}

void test_one_point_calibration_multi_process() {
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
  OnePointCalibrationMultiProcess calibrated(&inputs, &adjustments);
  TEST_ASSERT_EQUAL_FLOAT(2.5f, calibrated.readChannel(0));
  TEST_ASSERT_EQUAL_FLOAT(2.3f, calibrated.readChannel(1));
  TEST_ASSERT_EQUAL_FLOAT(-11.6f, calibrated.readChannel(2));
}

void test_two_point_calibration_process() {
  StateInput input(10.0f);
  // The triple point of water is actually 0.01f,
  // but we're going to make it 0.0f to make the math simpler.
  ConstantInput reference(Range(0.0f, 100.0f));
  ConstantInput offset(Range(10.0f, 90.0f));
  TwoPointCalibrationProcess calibrated(&input, &reference, &offset);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calibrated.read());
  input.write(90.0f);
  TEST_ASSERT_EQUAL_FLOAT(100.0f, calibrated.read());
  input.write(50.0f);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, calibrated.read());
  // Check outside the range too.
  input.write(100.0f);
  TEST_ASSERT_EQUAL_FLOAT(112.5f, calibrated.read());
  input.write(0.0f);
  TEST_ASSERT_EQUAL_FLOAT(-12.5f, calibrated.read());
}

void test_two_point_calibration_optional_process() {
  StateInput input((std::optional<float>)10.0f);
  ConstantInput reference(Range(0.0f, 100.0f));
  ConstantInput offset(Range(10.0f, 90.0f));
  TwoPointCalibrationOptionalProcess calibrated(&input, &reference, &offset);
  TEST_ASSERT_TRUE(calibrated.read().has_value());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calibrated.read().value());
  input.write(90.0f);
  TEST_ASSERT_EQUAL_FLOAT(100.0f, calibrated.read().value());
  input.write(50.0f);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, calibrated.read().value());
  // Check outside the range too.
  input.write(100.0f);
  TEST_ASSERT_EQUAL_FLOAT(112.5f, calibrated.read().value());
  input.write(0.0f);
  TEST_ASSERT_EQUAL_FLOAT(-12.5f, calibrated.read().value());
  input.write(std::nullopt);
  TEST_ASSERT_FALSE(calibrated.read().has_value());
}

void test_two_point_calibration_multi_process() {
  StateInput channel0(5.0f);
  StateInput channel1(10.0f);
  StateInput channel2(15.0f);
  std::map<int, Input<float>*> channels = {
    { 0, &channel0 },
    { 1, &channel1 },
    { 2, &channel2 }
  };
  MapMultiInput<int, float> inputs(&channels);
  std::map<int, Input<Range<float>>*> references = {
    { 0, new ConstantInput(Range(0.0f, 100.0f)) },
    { 1, new ConstantInput(Range(0.0f, 100.0f)) },
    { 2, new ConstantInput(Range(0.0f, 100.0f)) }
  };
  MapMultiInput referenceInputs(&references);
  std::map<int, Input<Range<float>>*> offsets = {
    { 0, new ConstantInput(Range(5.0f, 95.0f)) },
    { 1, new ConstantInput(Range(10.0f, 90.0f)) },
    { 2, new ConstantInput(Range(15.0f, 85.0f)) }
  };
  MapMultiInput offsetInputs(&offsets);
  TwoPointCalibrationMultiProcess<int, float> calibrated(&inputs, &referenceInputs, &offsetInputs);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calibrated.readChannel(0));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calibrated.readChannel(1));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, calibrated.readChannel(2));
  channel0.write(95.0f);
  channel1.write(90.0f);
  channel2.write(85.0f);
  TEST_ASSERT_EQUAL_FLOAT(100.0f, calibrated.readChannel(0));
  TEST_ASSERT_EQUAL_FLOAT(100.0f, calibrated.readChannel(1));
  TEST_ASSERT_EQUAL_FLOAT(100.0f, calibrated.readChannel(2));
  channel0.write(50.0f);
  channel1.write(50.0f);
  channel2.write(50.0f);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, calibrated.readChannel(0));
  TEST_ASSERT_EQUAL_FLOAT(50.0f, calibrated.readChannel(1));
  TEST_ASSERT_EQUAL_FLOAT(50.0f, calibrated.readChannel(2));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_translating_process);
  RUN_TEST(test_translating_optional_process);
  RUN_TEST(test_one_point_calibration_process);
  RUN_TEST(test_one_point_calibration_optional_process);
  RUN_TEST(test_one_point_calibration_multi_process);
  RUN_TEST(test_two_point_calibration_process);
  RUN_TEST(test_two_point_calibration_optional_process);
  RUN_TEST(test_two_point_calibration_multi_process);
  UNITY_END();
}