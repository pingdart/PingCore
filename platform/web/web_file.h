#pragma once

#include <memory>

#include <pingcore/file_drop.h>

namespace emscripten {
class val;
}

namespace pingcore::detail {

class FileAccessStore;
class FilePickerTransport;

[[nodiscard]] FileDropOffer ReadWebFileDropOffer(const emscripten::val& transfer);
[[nodiscard]] FileDropPreparation CaptureWebFileDrop(const emscripten::val& transfer);

[[nodiscard]] AppDirectories CreateWebAppDirectories();
std::shared_ptr<FileAccessStore> CreateWebFileAccessStore(const AppDirectories& directories);
[[nodiscard]] std::shared_ptr<FilePickerTransport> CreateWebFilePickerTransport();

} // namespace pingcore::detail
