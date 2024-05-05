#include <string>
#include <math.h>

#include <unity.h>
#include <input/CombiningProcesses.h>

void test_input_of_inputs() {
  std::vector<Input<int>*> inputs = {
    new ConstantInput(3),
    new ConstantInput(6),
    new ConstantInput(9)
  };
  InputOfInputs<int> inputOfInputs(&inputs);
  TEST_ASSERT_EQUAL(3, inputOfInputs.read().size());
  TEST_ASSERT_EQUAL(3, inputOfInputs.read()[0]);
  inputs.push_back(new ConstantInput(12));
  TEST_ASSERT_EQUAL(4, inputOfInputs.read().size());
  TEST_ASSERT_EQUAL(12, inputOfInputs.read()[3]);
}

void test_input_of_mapped_inputs() {
  std::map<int, Input<int>*> inputs = {
    { 0, new ConstantInput(3) },
    { 1, new ConstantInput(6) },
    { 2, new ConstantInput(9) }
  };
  InputOfMappedInputs<int, int> inputOfInputs(&inputs);
  TEST_ASSERT_EQUAL(3, inputOfInputs.read().size());
  TEST_ASSERT_EQUAL(3, inputOfInputs.read()[0]);
  inputs.insert({3, new ConstantInput(12) });
  TEST_ASSERT_EQUAL(4, inputOfInputs.read().size());
  TEST_ASSERT_EQUAL(12, inputOfInputs.read()[3]);
}

void test_merging_range_process() {
  StateInput min(3);
  StateInput max(5);
  MergingRangeProcess<int> range(&min, &max);
  TEST_ASSERT_EQUAL(3, range.read().min);
  TEST_ASSERT_EQUAL(5, range.read().max);
  min.write(2);
  max.write(6);
  TEST_ASSERT_EQUAL(2, range.read().min);
  TEST_ASSERT_EQUAL(6, range.read().max);
}

void test_merging_2_process() {
  StateInput left(1);
  StateInput right(3.5);
  Merging2Process<int, double> input(&left, &right);
  TEST_ASSERT_EQUAL(1, std::get<0>(input.read()));
  TEST_ASSERT_EQUAL(3.5, std::get<1>(input.read()));
  left.write(5600);
  right.write(0.000007);
  TEST_ASSERT_EQUAL(5600, std::get<0>(input.read()));
  TEST_ASSERT_EQUAL(0.000007, std::get<1>(input.read()));
}

void test_merging_2_not_empty_process() {
  StateInput<std::optional<int>> left(1);
  StateInput<std::optional<double>> right(std::nullopt);
  Merging2NotEmptyProcess<int, double> input(&left, &right);
  TEST_ASSERT_FALSE(input.read().has_value());
  right.write(1.6677);
  TEST_ASSERT_TRUE(input.read().has_value());
  TEST_ASSERT_EQUAL(1.6677, std::get<1>(input.read().value()));
  left.write(std::nullopt);
  TEST_ASSERT_FALSE(input.read().has_value());
}

void test_fold_process() {
  std::vector<Input<int>*> inputs = {
    new ConstantInput(3),
    new ConstantInput(6),
    new ConstantInput(9)
  };
  StateInput<std::string> initialValueInput("My favourite numbers are ");
  FoldProcess<int, std::string> doNumbers(&inputs, [](std::string acc, int v) { return acc + std::to_string(v); }, &initialValueInput);
  TEST_ASSERT_EQUAL_STRING("My favourite numbers are 369", doNumbers.read().c_str());
  initialValueInput.write("Your winning lotto numbers are ");
  TEST_ASSERT_EQUAL_STRING("Your winning lotto numbers are 369", doNumbers.read().c_str());
  inputs.push_back(new ConstantInput(0));
  TEST_ASSERT_EQUAL_STRING("Your winning lotto numbers are 3690", doNumbers.read().c_str());
}

void test_reduce_process() {
  std::vector<Input<int>*> inputs;
  ReduceProcess<int> findMin(&inputs, [](int a, int b) { return std::min(a, b); });
  try {
    int value = findMin.read();
    TEST_FAIL_MESSAGE("should've thrown error trying to reduce empty vector");
  } catch (std::invalid_argument e) {
    TEST_PASS();
  }
  inputs.push_back(new ConstantInput(3));
  TEST_ASSERT_EQUAL(3, findMin.read());
  inputs.push_back(new ConstantInput(1));
  TEST_ASSERT_EQUAL(1, findMin.read());
  inputs.push_back(new ConstantInput(2));
  TEST_ASSERT_EQUAL(1, findMin.read());
}

void test_map_process() {
  std::vector<Input<int>*> inputs;
  MapProcess<int, int> doubleIt(&inputs, [](int v) { return v * 2; });
  TEST_ASSERT_EQUAL(0, doubleIt.read().size());
  inputs.push_back(new ConstantInput(3));
  TEST_ASSERT_EQUAL(1, doubleIt.read().size());
  TEST_ASSERT_EQUAL(6, doubleIt.read()[0]);
  StateInput changeable(4);
  inputs.push_back(&changeable);
  TEST_ASSERT_EQUAL(2, doubleIt.read().size());
  TEST_ASSERT_EQUAL(8, doubleIt.read()[1]);
  changeable.write(1);
  TEST_ASSERT_EQUAL(2, doubleIt.read()[1]);
}

void test_filter_process() {
  StateInput changeable(12);
  std::vector<Input<int>*> inputs = {
    new ConstantInput(3),
    new ConstantInput(11),
    &changeable,
    new ConstantInput(4)
  };
  FilterProcess<int> onlySingleDigits(&inputs, [](int v) { return v < 10; });
  TEST_ASSERT_EQUAL(2, onlySingleDigits.read().size());
  changeable.write(5);
  TEST_ASSERT_EQUAL(3, onlySingleDigits.read().size());
  inputs.push_back(new ConstantInput(165));
  TEST_ASSERT_EQUAL(3, onlySingleDigits.read().size());
  inputs.push_back(new ConstantInput(1));
  TEST_ASSERT_EQUAL(4, onlySingleDigits.read().size());
  TEST_ASSERT_EQUAL(3, onlySingleDigits.read()[0]);
  TEST_ASSERT_EQUAL(5, onlySingleDigits.read()[1]);
  TEST_ASSERT_EQUAL(4, onlySingleDigits.read()[2]);
  TEST_ASSERT_EQUAL(1, onlySingleDigits.read()[3]);
}

void test_avg_process() {
  StateInput changeable(7.0f);
  std::vector<Input<float>*> inputs = {
    new ConstantInput(3.0f),
    new ConstantInput(5.0f),
    &changeable
  };
  AvgProcess<float> avgInput(&inputs);
  TEST_ASSERT_EQUAL_FLOAT(5.0f, avgInput.read());
  inputs.push_back(new ConstantInput(2.0f));
  TEST_ASSERT_EQUAL_FLOAT(4.25f, avgInput.read());
  changeable.write(3.0f);
  TEST_ASSERT_EQUAL_FLOAT(3.25f, avgInput.read());
}

void test_min_process() {
  std::vector<Input<int>*> inputs = {
    new ConstantInput(1),
    new ConstantInput(3),
    new ConstantInput(5)
  };
  MinProcess<int> minInput(&inputs);
  TEST_ASSERT_EQUAL(1, minInput.read());
}

void test_max_process() {
  std::vector<Input<int>*> inputs = {
    new ConstantInput(1),
    new ConstantInput(3),
    new ConstantInput(5)
  };
  MaxProcess<int> maxInput(&inputs);
  TEST_ASSERT_EQUAL(5, maxInput.read());
}

void test_input_switcher() {
  StateInput<int> changeable(6);
  std::map<int, Input<int>*> inputMap = {
    { 0, new ConstantInput(3) },
    { 1, &changeable },
    { 2, new ConstantInput(9) }
  };
  StateInput<int> switchInput(0);
  InputSwitcher<int, int> switcher(&inputMap, &switchInput);
  TEST_ASSERT_EQUAL(3, switcher.read());
  switchInput.write(1);
  TEST_ASSERT_EQUAL(6, switcher.read());
  changeable.write(11);
  TEST_ASSERT_EQUAL(11, switcher.read());
}

void test_not_empty_process() {
  std::vector<Input<std::optional<int>>*> inputs = {
    new ConstantInput<std::optional<int>>(3),
    new ConstantInput<std::optional<int>>(6),
    new ConstantInput<std::optional<int>>(std::nullopt),
    new ConstantInput<std::optional<int>>(9),
  };
  NotEmptyProcess<int> notEmpties(&inputs);
  TEST_ASSERT_EQUAL(3, notEmpties.read().size());
  TEST_ASSERT_EQUAL(9, notEmpties.read()[2]);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_input_of_inputs);
  RUN_TEST(test_input_of_mapped_inputs);
  RUN_TEST(test_merging_range_process);
  RUN_TEST(test_merging_2_process);
  RUN_TEST(test_merging_2_not_empty_process);
  RUN_TEST(test_fold_process);
  RUN_TEST(test_reduce_process);
  RUN_TEST(test_map_process);
  RUN_TEST(test_filter_process);
  RUN_TEST(test_avg_process);
  RUN_TEST(test_min_process);
  RUN_TEST(test_max_process);
  RUN_TEST(test_input_switcher);
  RUN_TEST(test_not_empty_process);
  UNITY_END();
}