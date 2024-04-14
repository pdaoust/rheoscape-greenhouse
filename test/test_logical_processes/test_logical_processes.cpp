#include <unity.h>

#include <input/LogicalProcesses.h>

void test_and_process() {
  StateInput a(true);
  StateInput b(true);
  AndProcess aAndB(&a, &b);
  TEST_ASSERT_TRUE(aAndB.read());
  a.write(false);
  TEST_ASSERT_FALSE(aAndB.read());
  b.write(false);
  TEST_ASSERT_FALSE(aAndB.read());
  a.write(true);
  TEST_ASSERT_FALSE(aAndB.read());
}

void test_or_process() {
  StateInput a(true);
  StateInput b(true);
  OrProcess aOrB(&a, &b);
  TEST_ASSERT_TRUE(aOrB.read());
  a.write(false);
  TEST_ASSERT_TRUE(aOrB.read());
  b.write(false);
  TEST_ASSERT_FALSE(aOrB.read());
  a.write(true);
  TEST_ASSERT_TRUE(aOrB.read());
}

void test_xor_process() {
  StateInput a(true);
  StateInput b(true);
  XorProcess aXorB(&a, &b);
  TEST_ASSERT_FALSE(aXorB.read());
  a.write(false);
  TEST_ASSERT_TRUE(aXorB.read());
  b.write(false);
  TEST_ASSERT_FALSE(aXorB.read());
  a.write(true);
  TEST_ASSERT_TRUE(aXorB.read());
}

void test_not_process() {
  StateInput a(true);
  NotProcess notA(&a);
  TEST_ASSERT_FALSE(notA.read());
  a.write(false);
  TEST_ASSERT_TRUE(notA.read());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_and_process);
  RUN_TEST(test_or_process);
  RUN_TEST(test_xor_process);
  RUN_TEST(test_not_process);
  UNITY_END();
}