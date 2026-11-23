#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <variant>

#include <pingcore/platform_registry.h>
#include <pingcore/app.h>

namespace pingcore::example {

namespace timer {

inline constexpr char type[] = "example/Timer";

} // namespace timer

class TimerService {
public:
  virtual ~TimerService() = default;

  TimerService(const TimerService&) = delete;
  TimerService& operator=(const TimerService&) = delete;
  TimerService(TimerService&&) = delete;
  TimerService& operator=(TimerService&&) = delete;

  virtual PlatformRequestId Start(
      std::chrono::milliseconds interval,
      std::function<void(std::uint64_t)> tick_handler,
      std::function<void(PlatformResult<std::uint64_t>)> completion
  ) = 0;
  virtual PlatformRequestId Stop(std::function<void(PlatformResult<std::monostate>)> completion) = 0;
  virtual bool Cancel(PlatformRequestId request) = 0;

protected:
  TimerService() = default;
};

std::shared_ptr<TimerService> UseTimer();
void InstallTimer(ApplicationContext& root);

} // namespace pingcore::example
