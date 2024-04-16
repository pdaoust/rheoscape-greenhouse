#include <functional>
#include <memory>

#include <unity.h>

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
  auto runnable = std::make_shared<LambdaRunnable>([&didRun]() { didRun = true; });
  Runner::registerRunnable(runnable);
  Runner::run();
  TEST_ASSERT_TRUE(didRun);
}

void test_runner_doesnt_run_deallocated_runnable_and_doesnt_crash_either() {
  bool didRun = false;
  {
    auto runnable = std::make_shared<LambdaRunnable>([&didRun]() { didRun = true; });
    Runner::registerRunnable(runnable);
  }
  Runner::run();
  TEST_ASSERT_FALSE(didRun);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_runner_runs_runnable);
  RUN_TEST(test_runner_doesnt_run_deallocated_runnable_and_doesnt_crash_either);
  UNITY_END();
}