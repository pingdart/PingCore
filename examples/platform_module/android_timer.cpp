#include "timer.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include <pingcore/android/platform_registry.h>

namespace {

constexpr char platform_timer_class[] = "org.pingcore.examples.platformmodule.PlatformTimer";

struct TimerTick : pingcore::Event<void(std::uint64_t)> {
  static constexpr char Name[] = "tick";
};

struct AndroidTimerCallbacks {
  std::function<void(std::uint64_t)> tick;
};

class AndroidTimerService final : public pingcore::example::TimerService {
public:
  explicit AndroidTimerService(pingcore::PlatformChannel channel)
      : channel_(std::move(channel)), callbacks_(std::make_shared<AndroidTimerCallbacks>()) {
    channel_.On<TimerTick>([callbacks = callbacks_](std::uint64_t tick) {
      if (callbacks->tick) {
        callbacks->tick(tick);
      }
    });
  }

  ~AndroidTimerService() override {
    callbacks_->tick = {};
    channel_.Close();
  }

  pingcore::PlatformRequestId Start(std::chrono::milliseconds interval, std::function<void(std::uint64_t)> handler,
                                   std::function<void(pingcore::PlatformResult<std::uint64_t>)> completion) override {
    if (interval <= std::chrono::milliseconds::zero()) {
      throw std::invalid_argument("PingCore example timer interval is outside the Android timer range");
    }
    if (!handler || !completion) {
      throw std::invalid_argument("PingCore example timer callbacks must not be empty");
    }
    callbacks_->tick = std::move(handler);
    return channel_.Invoke<std::uint64_t>("start", interval.count(), std::move(completion));
  }

  pingcore::PlatformRequestId Stop(std::function<void(pingcore::PlatformResult<std::monostate>)> completion) override {
    if (!completion) {
      throw std::invalid_argument("PingCore example timer completion must not be empty");
    }
    callbacks_->tick = {};
    return channel_.Invoke<std::monostate>("stop", std::move(completion));
  }

  bool Cancel(pingcore::PlatformRequestId request) override {
    const bool cancelled = channel_.Cancel(request);
    if (cancelled) {
      callbacks_->tick = {};
    }
    return cancelled;
  }

private:
  pingcore::PlatformChannel channel_;
  std::shared_ptr<AndroidTimerCallbacks> callbacks_;
};

} // namespace

namespace pingcore::example {

void InstallTimer(ApplicationContext& root) {
  android::JavaPlatformModuleFactory<std::shared_ptr<TimerService>> factory;
  factory.class_name = platform_timer_class;
  factory.create = [](PlatformChannel channel) {
    return std::static_pointer_cast<TimerService>(std::make_shared<AndroidTimerService>(std::move(channel)));
  };
  root.RegisterPlatformModule<std::shared_ptr<TimerService>>(timer::type, std::move(factory));
}

} // namespace pingcore::example
