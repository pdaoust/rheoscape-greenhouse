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

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_translating_process);
  RUN_TEST(test_translating_optional_process);
  UNITY_END();
}