// include/pingdart/rhi/metal/metal_pipeline.h
// PKG-114: PingCore Native Metal Pipeline State Object
#pragma once
#include <pingdart/rhi/i_pipeline_state.h>
#include <memory>

namespace pingdart::rhi::metal {

class MetalPipelineState final : public IPipelineState {
public:
    MetalPipelineState(void* mtl_pipeline_state, const PipelineDescriptor& desc);
    ~MetalPipelineState() override;

    MetalPipelineState(const MetalPipelineState&) = delete;
    MetalPipelineState& operator=(const MetalPipelineState&) = delete;

    [[nodiscard]] const PipelineDescriptor& descriptor() const noexcept override {
        return descriptor_;
    }

    [[nodiscard]] void* native_pipeline_state() const noexcept {
        return native_pipeline_state_;
    }

private:
    void* native_pipeline_state_{nullptr}; // id<MTLRenderPipelineState>
    PipelineDescriptor descriptor_;
};

} // namespace pingdart::rhi::metal
