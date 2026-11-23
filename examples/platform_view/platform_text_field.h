#pragma once

#include <string>

#include <pingcore/event.h>
#include <pingcore/platform_registry.h>
#include <pingcore/app.h>
#include <pingcore/view.h>

namespace pingcore::example {

namespace platform_text_field {

inline constexpr char type[] = "example/PlatformTextField";
} // namespace platform_text_field

struct PlatformTextFieldProperties {
  std::string text;

  [[nodiscard]] static PlatformPayload Encode(const PlatformTextFieldProperties& properties) {
    return PlatformPayload(properties.text);
  }

  bool operator==(const PlatformTextFieldProperties&) const = default;
};

struct PlatformTextFieldEvents {
  struct Changed : Event<void(const std::string&)> {
    static constexpr char Name[] = "changed";
  };
};

View PlatformTextField(std::string value);
void InstallPlatformTextField(ApplicationContext& root);

} // namespace pingcore::example
