#ifndef RHEOSCAPE_RUNNABLE_H
#define RHEOSCAPE_RUNNABLE_H

#include <functional>
#include <vector>

#include <helpers/string_format.h>

class Runner {
  private:
    inline static std::vector<std::function<void()>> _callbacks;
    inline static std::string _message;

  public:
    static void registerCallback(std::function<void()> callback) {
      Runner::_callbacks.push_back(callback);
    }

    static void run() {
      Runner::_message = "starting run";
      for (int i = 0; i < _callbacks.size(); i ++) {
        Runner::_callbacks[i]();
      }
      Runner::_message = "finished run";
    }
};

#endif