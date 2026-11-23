#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include <pingcore/external_texture.h>
#include <pingcore/app.h>

namespace pingcore::detail {

class ExternalTextureFrameRequester final : public std::enable_shared_from_this<ExternalTextureFrameRequester> {
public:
  ExternalTextureFrameRequester(UiWindow& ui_window, UiThreadDispatcher dispatch_to_ui_thread)
      : ui_window_(&ui_window), dispatch_to_ui_thread_(std::move(dispatch_to_ui_thread)) {}

  [[nodiscard]] bool CanRequestFrames();
  void RequestFrame();
  void SetActive(const std::shared_ptr<ExternalTexture>& texture, bool active);
  void Close() noexcept;

private:
  UiWindow* ui_window_;
  UiThreadDispatcher dispatch_to_ui_thread_;
  std::mutex mutex_;
  bool request_pending_ = false;
  bool closed_ = false;
};

} // namespace pingcore::detail
