#include <unity.h>

#include <functional>

#include <Runnable.h>

class LambdaRunnable : public Runnable {
  private:
    std::function<void()> _callback;

  public:
    LambdaRunnable(std::function<void()> callback)
    : _callback(callback)
    { }

    virtual void run() {
      _callback();
    }
};

void test_runner_runs_runnable() {
  bool didRun = false;
  LambdaRunnable runnable([&didRun]() { didRun = true; });
  Runner::run();
  TEST_ASSERT_TRUE(didRun);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_runner_runs_runnable);
  UNITY_END();
}