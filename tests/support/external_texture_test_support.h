#pragma once

#include <memory>

#include <pingcore/external_texture.h>

namespace pingcore::test {

class ExternalTextureTestTexture final : public ExternalTexture {
public:
  explicit ExternalTextureTestTexture(Size intrinsic_size) : ExternalTexture(intrinsic_size) {}

  void PublishFrame() {
    NotifyFrameAvailable();
  }
};

inline std::shared_ptr<ExternalTexture> MakeTestExternalTexture(Size intrinsic_size) {
  return std::make_shared<ExternalTextureTestTexture>(intrinsic_size);
}

} // namespace pingcore::test
