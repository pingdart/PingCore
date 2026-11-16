// src/core/compiler.cpp
#include <pingdart/core/compiler.h>

namespace pingdart::core {

namespace {

Status MapPdStatus(pd_status_t status, std::string_view default_msg) {
    switch (status) {
        case PD_OK:
            return Status::Ok();
        case PD_INVALID_ARGUMENT:
            return Status::InvalidArgument(std::string(default_msg));
        case PD_OUT_OF_MEMORY:
            return Status::ResourceExhausted(std::string(default_msg));
        case PD_COMPILER_ERROR:
            return Status(StatusCode::InvalidArgument, std::string(default_msg));
        case PD_INTERNAL_ERROR:
            return Status::Internal(std::string(default_msg));
    }
    return Status::Internal("Unknown compiler FFI status");
}

} // namespace

uint32_t Compiler::abi_version() noexcept {
    return pd_compiler_get_abi_version();
}

Result<Compiler> Compiler::create() {
    if (abi_version() != PD_COMPILER_ABI_VERSION) {
        return Status::FailedPrecondition("Compiler ABI version mismatch");
    }

    pd_compiler_ctx_t* ctx = nullptr;
    pd_status_t status = pd_compiler_create(&ctx);
    if (status != PD_OK || !ctx) {
        return MapPdStatus(status, "Failed to initialize compiler context");
    }

    return Compiler(ctx);
}

Compiler::~Compiler() {
    if (ctx_) {
        pd_compiler_destroy(ctx_);
        ctx_ = nullptr;
    }
}

Compiler::Compiler(Compiler&& other) noexcept : ctx_(other.ctx_) {
    other.ctx_ = nullptr;
}

Compiler& Compiler::operator=(Compiler&& other) noexcept {
    if (this != &other) {
        if (ctx_) {
            pd_compiler_destroy(ctx_);
        }
        ctx_ = other.ctx_;
        other.ctx_ = nullptr;
    }
    return *this;
}

Result<std::vector<uint8_t>> Compiler::compile(std::string_view source) {
    if (!ctx_) {
        return Status::FailedPrecondition("Compiler context is null");
    }

    pd_buffer_t out_buf{nullptr, 0};
    pd_status_t status = pd_compiler_compile(
        ctx_,
        reinterpret_cast<const uint8_t*>(source.data()),
        source.size(),
        &out_buf
    );

    if (status != PD_OK) {
        if (out_buf.data) {
            pd_compiler_free_buffer(&out_buf);
        }
        return MapPdStatus(status, "Compilation failed");
    }

    std::vector<uint8_t> result;
    if (out_buf.data && out_buf.length > 0) {
        result.assign(out_buf.data, out_buf.data + out_buf.length);
    }

    // Explicit deallocation through Rust FFI export
    pd_compiler_free_buffer(&out_buf);

    return result;
}

} // namespace pingdart::core
