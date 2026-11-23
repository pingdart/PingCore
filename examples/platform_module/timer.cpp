#include "timer.h"

namespace pingcore::example {

std::shared_ptr<TimerService> UseTimer() {
  return UseService<TimerService>();
}

} // namespace pingcore::example
