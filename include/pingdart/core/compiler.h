// include/pingdart/core/compiler.h
#pragma once
#include <pingdart/core/pd_compiler_abi.h>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>
#include <string_view>
#include <vector>

namespace pingdart::core {

class Compiler {
public:
    static Result<Compiler> create();

    ~Compiler();

    Compiler(Compiler&& other) noexcept;
    Compiler& operator=(Compiler&& other) noexcept;

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;

    // Compiles PingDart source code into a bytecode vector
    [[nodiscard]] Result<std::vector<uint8_t>> compile(std::string_view source);

    [[nodiscard]] static uint32_t abi_version() noexcept;

private:
    explicit Compiler(pd_compiler_ctx_t* ctx) noexcept : ctx_(ctx) {}

    pd_compiler_ctx_t* ctx_{nullptr};
};

} // namespace pingdart::core
