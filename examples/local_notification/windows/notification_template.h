#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <pingcore/platform_registry.h>

namespace local_notification_example {

std::optional<std::string> BuildNotificationTemplate(std::string_view template_identifier, std::string_view title,
                                                    std::string_view body, const pingcore::PlatformPayload& data);

} // namespace local_notification_example
