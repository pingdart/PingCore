#pragma once

#include <pingcore/macos/external_texture.h>

@class HUXExternalTexture;

namespace pingcore::macos::detail {

HUXExternalTexture* WrapExternalTexture(std::shared_ptr<ExternalTexture> texture);
std::shared_ptr<ExternalTexture> UnwrapExternalTexture(HUXExternalTexture* texture);

} // namespace pingcore::macos::detail
