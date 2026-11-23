#pragma once

#include <functional>
#include <memory>

#ifdef __OBJC__
@class NSURL;
@class NSError;
@class UIDocumentPickerViewController;
@class HUXPresentationCoordinator;
@class PingCoreView;
#endif

namespace pingcore {
class FileReference;
struct AppDirectories;
} // namespace pingcore

namespace pingcore::detail {

class FileAccessStore;
class FilePickerTransport;

[[nodiscard]] AppDirectories CreateIosAppDirectories();
std::shared_ptr<FileAccessStore> CreateIosFileAccessStore(const AppDirectories& directories);

#ifdef __OBJC__
[[nodiscard]] std::shared_ptr<FilePickerTransport>
CreateIosFilePickerTransport(HUXPresentationCoordinator* coordinator);
// Called on the main thread with a nonempty completion; native transitions finish before releasing the picker slot.
void DismissIosFilePicker(UIDocumentPickerViewController* picker, std::function<void()> completion);
void InstallIosFileDrop(PingCoreView* view);
[[nodiscard]] FileReference MakeIosFileReference(NSURL* url, bool writable = true);
[[nodiscard]] FileReference MakeCopiedIosFileReference(NSURL* url, NSError** error = nullptr);
#endif

} // namespace pingcore::detail
