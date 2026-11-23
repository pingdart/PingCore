#include "platform_text_field.h"

#include <emscripten/val.h>

#include <pingcore/web/platform_registry.h>

namespace {

constexpr char platform_text_field_factory_name[] = "pingcoreExamplePlatformTextFieldFactory";

} // namespace

namespace pingcore::example {

void InstallPlatformTextField(ApplicationContext& root) {
  root.RegisterPlatformView<PlatformTextFieldProperties>(
      platform_text_field::type,
      web::JavaScriptPlatformViewFactory<PlatformTextFieldProperties>{
          .factory = emscripten::val::module_property(platform_text_field_factory_name),
      });
}

} // namespace pingcore::example
