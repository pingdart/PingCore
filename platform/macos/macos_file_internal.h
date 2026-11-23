#pragma once

#include <functional>
#include <memory>

#include <pingcore/file_drop.h>

#ifdef __OBJC__
@class NSWindow;
@class NSURL;
@class NSPasteboard;
#endif

namespace pingcore {
class FileReference;
} // namespace pingcore

namespace pingcore::detail {

class FileAccessStore;
class FilePickerTransport;

[[nodiscard]] AppDirectories CreateMacAppDirectories();
std::shared_ptr<FileAccessStore> CreateMacFileAccessStore(const AppDirectories& directories);

#ifdef __OBJC__
[[nodiscard]] std::shared_ptr<FilePickerTransport>
CreateMacFilePickerTransport(std::function<NSWindow*()> window_provider);
[[nodiscard]] FileDropPreparation CaptureMacFileDrop(NSPasteboard* pasteboard);
[[nodiscard]] FileReference MakeMacFileReference(NSURL* url, bool writable = true);
#endif

} // namespace pingcore::detail
