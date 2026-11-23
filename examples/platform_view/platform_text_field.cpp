#include "platform_text_field.h"

#include <string>
#include <utility>

#include <pingcore/platform_registry.h>

namespace pingcore::example {

View PlatformTextField(std::string value) {
  return PlatformView(platform_text_field::type, PlatformTextFieldProperties{std::move(value)});
}

} // namespace pingcore::example
