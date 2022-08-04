#pragma once

#include "Timer.h"

#include <functional>
#include <map>

class TimerExtent {
private:

  TimerExtent();
  ~TimerExtent();

public:
  static TimerExtent* m_instance;

  static TimerExtent& instance();

  long setTimeout(std::function<void()> function, int delay);
  void clearTimeout(long timerId);

private:
  long   m_nextTimerId;
  std::map<long, Timer*> m_timers;
};

long setTimeout(std::function<void()> function, int delay);
void clearTimeout(long timerId);
