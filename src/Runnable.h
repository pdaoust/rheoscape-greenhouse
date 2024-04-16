#ifndef RHEOSCAPE_RUNNABLE_H
#define RHEOSCAPE_RUNNABLE_H

#include <functional>
#include <memory>
#include <vector>

#include <helpers/string_format.h>

class Runnable {
  public:
    virtual void run() = 0;
};

class Runner {
  private:
    inline static std::vector<std::weak_ptr<Runnable>> _runnables;

  public:
    static void registerRunnable(std::weak_ptr<Runnable> runnable) {
      Runner::_runnables.push_back(runnable);
    }

    static void run() {
      for (int i = 0; i < _runnables.size(); i ++) {
        if (!_runnables[i].expired()) {
          _runnables[i].lock()->run();
        }
      }
    }
};

#endif