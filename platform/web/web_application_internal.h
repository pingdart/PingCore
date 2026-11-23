#pragma once

#include <memory>

#include <pingcore/clipboard.h>

namespace pingcore::detail {

class PermissionTransport;

[[nodiscard]] std::shared_ptr<PermissionTransport> CreateWebPermissionTransport();
Task<bool> WriteWebClipboardAsync(const ClipboardItem& item);
Task<IoResult<ClipboardItem>> ReadWebClipboardAsync(const ClipboardReadOptions& options);

} // namespace pingcore::detail
