#pragma once

#include <atomic>
#include <functional>

class Timer {
  std::atomic<bool> active{ true };

public:
  void setTimeout(std::function<void()> function, int delay);
  void setInterval(std::function<void()> function, int interval);
  void stop();
};
