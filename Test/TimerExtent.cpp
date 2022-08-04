#include "TimerExtent.h"
#include "Timer.h"

#include <functional>

TimerExtent* TimerExtent::m_instance = nullptr;

TimerExtent::TimerExtent() : m_nextTimerId(1)
{}

TimerExtent::~TimerExtent()
{}

TimerExtent&
TimerExtent::instance()
{
  if (!TimerExtent::m_instance) {
    TimerExtent::m_instance = new TimerExtent();
  }
  return *TimerExtent::m_instance;
}

long
TimerExtent::setTimeout(std::function<void()> function, int delay)
{
  long timerId = m_nextTimerId++;
  Timer* t = new Timer();
  m_timers[timerId] = t;
  t->setTimeout(function, delay);
  return timerId;
}

void
TimerExtent::clearTimeout(long timerId) {
  auto it = m_timers.find(timerId);
  if (it != m_timers.end()) {
    delete m_timers[timerId];
    m_timers.erase(it);
  }
}

long setTimeout(std::function<void()> function, int delay)
{
  return TimerExtent::instance().setTimeout(function, delay);
}

void clearTimeout(long timerId)
{
  TimerExtent::instance().clearTimeout(timerId);
}
