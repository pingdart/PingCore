#include "platform_text_field.h"

#include <pingcore/android/platform_registry.h>

namespace {

constexpr char platform_text_field_class[] = "org.pingcore.examples.platformview.PlatformTextField";

} // namespace

namespace pingcore::example {

void InstallPlatformTextField(ApplicationContext& root) {
  root.RegisterPlatformView<PlatformTextFieldProperties>(platform_text_field::type,
                                                         android::JavaPlatformViewFactory<PlatformTextFieldProperties>{
                                                             .class_name = platform_text_field_class,
                                                         });
}

} // namespace pingcore::example
