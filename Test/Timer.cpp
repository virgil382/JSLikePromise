#include "Timer.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

/*
long setTimeout(auto function, int delay) {
  return TimerExtent::instance().setTimeout(function, delay);
}

void clearTimeout(long timerId) {
  return TimerExtent::instance().clearTimeout(timerId);
}
*/

void Timer::setTimeout(std::function<void()> function, int delay) {
  active = true;
  std::thread t([=]() {
    if (!active.load()) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    if (!active.load()) return;
    function();
    });
  t.detach();
}

void Timer::setInterval(std::function<void()> function, int interval) {
  active = true;
  std::thread t([=]() {
    while (active.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
      if (!active.load()) return;
      function();
    }
    });
  t.detach();
}

void Timer::stop() {
  active = false;
}
