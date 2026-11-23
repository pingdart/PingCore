#pragma once

#include <pingcore/ios/external_texture.h>

@class HUXExternalTexture;

namespace pingcore::ios::detail {

HUXExternalTexture* WrapExternalTexture(std::shared_ptr<ExternalTexture> texture);
std::shared_ptr<ExternalTexture> UnwrapExternalTexture(HUXExternalTexture* texture);

} // namespace pingcore::ios::detail
