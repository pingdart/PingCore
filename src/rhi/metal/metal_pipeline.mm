// src/rhi/metal/metal_pipeline.mm
// PKG-114: PingCore Native Metal Pipeline Implementation
#include <pingdart/rhi/metal/metal_pipeline.h>
#import <Metal/Metal.h>

namespace pingdart::rhi::metal {

MetalPipelineState::MetalPipelineState(void* mtl_pipeline_state, const PipelineDescriptor& desc)
    : native_pipeline_state_(mtl_pipeline_state)
    , descriptor_(desc)
{
    if (native_pipeline_state_) {
        CFRetain(native_pipeline_state_);
    }
}

MetalPipelineState::~MetalPipelineState() {
    if (native_pipeline_state_) {
        CFRelease(native_pipeline_state_);
        native_pipeline_state_ = nullptr;
    }
}

} // namespace pingdart::rhi::metal
