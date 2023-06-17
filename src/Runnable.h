#ifndef PAULDAOUST_RUNNABLE_H
#define PAULDAOUST_RUNNABLE_H

#include <vector>
#include <Timer.h>

class AbstractRunnable {
  public:
    virtual void run() { }
};

class Runner {
  private:
    std::vector<AbstractRunnable*> _runnables;
    RepeatTimer _timer;

  public:
    Runner(unsigned long tickInterval = 1)
    : _timer(RepeatTimer(
        millis(),
        tickInterval,
        [this]() {
          for (int i = 0; i < _runnables.size(); i ++) {
            _runnables[i]->run();
          }
        }
      ))
    { }

    void registerRunnable(AbstractRunnable* runnable) {
      _runnables.push_back(runnable);
    }

    void run() {
      _timer.tick();
    }
};

class Runnable : public AbstractRunnable {
  protected:
    Runnable(Runner runner) {
      runner.registerRunnable(this);
    }
};

#endif