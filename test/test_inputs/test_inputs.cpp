#include <unity.h>
#include <input/Input.h>

void test_function_input() {
  Input<int>* input = new FunctionInput<int>([]() { return 3; });
  int value = input->read();
  TEST_ASSERT_EQUAL(3, value);
}

void test_constant_input() {
  Input<int>* input = new ConstantInput(3);
  int value = input->read();
  TEST_ASSERT_EQUAL(3, value);
}

void test_make_range_constant_input() {
  auto input = makeRangeConstantInput<int>(3, 10);
  Range<int> value = input.read();
  TEST_ASSERT_EQUAL(3, value.min);
  TEST_ASSERT_EQUAL(10, value.max);
}

void test_state_input() {
  StateInput input(3);
  TEST_ASSERT_EQUAL(3, input.read());
  input.write(5);
  TEST_ASSERT_EQUAL(5, input.read());
}

void test_pointer_input() {
  int value = 3;
  PointerInput input(&value);
  TEST_ASSERT_EQUAL(3, input.read());
  value = 5;
  TEST_ASSERT_EQUAL(5, input.read());
}

void test_multi_input() {
  std::map<int, Input<int>*> inputs = {
    { 0, new ConstantInput(3) },
    { 1, new ConstantInput(6) },
    { 2, new ConstantInput(9) }
  };
  MapMultiInput<int, int> input(inputs);
  TEST_ASSERT_EQUAL(3, input.readChannel(0));
  TEST_ASSERT_EQUAL(6, input.readChannel(1));
  TEST_ASSERT_EQUAL(9, input.readChannel(2));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_function_input);
  RUN_TEST(test_constant_input);
  RUN_TEST(test_state_input);
  RUN_TEST(test_make_range_constant_input);
  RUN_TEST(test_pointer_input);
  RUN_TEST(test_multi_input);
  UNITY_END();
}