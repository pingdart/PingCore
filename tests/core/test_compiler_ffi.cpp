// tests/core/test_compiler_ffi.cpp
#include <gtest/gtest.h>
#include <pingdart/core/pd_compiler_abi.h>
#include <pingdart/core/compiler.h>
#include <cstring>
#include <vector>

using namespace pingdart::core;

TEST(CompilerFfiTest, AbiVersionAgreement) {
    uint32_t version = pd_compiler_get_abi_version();
    EXPECT_EQ(version, PD_COMPILER_ABI_VERSION);
    EXPECT_EQ(Compiler::abi_version(), PD_COMPILER_ABI_VERSION);
}

TEST(CompilerFfiTest, LifecycleCreateAndDestroy) {
    pd_compiler_ctx_t* ctx = nullptr;
    pd_status_t status = pd_compiler_create(&ctx);
    ASSERT_EQ(status, PD_OK);
    ASSERT_NE(ctx, nullptr);

    pd_compiler_destroy(ctx);
}

TEST(CompilerFfiTest, NullPointerArgumentsReturnDocumentedErrors) {
    // Null out_context
    pd_status_t status = pd_compiler_create(nullptr);
    EXPECT_EQ(status, PD_INVALID_ARGUMENT);

    // Null context to compile
    const uint8_t src[] = "test";
    pd_buffer_t buf{nullptr, 0};
    status = pd_compiler_compile(nullptr, src, sizeof(src), &buf);
    EXPECT_EQ(status, PD_INVALID_ARGUMENT);

    // Null out_buffer to compile
    pd_compiler_ctx_t* ctx = nullptr;
    ASSERT_EQ(pd_compiler_create(&ctx), PD_OK);
    status = pd_compiler_compile(ctx, src, sizeof(src), nullptr);
    EXPECT_EQ(status, PD_INVALID_ARGUMENT);

    // Null buffer to free_buffer should not crash
    pd_compiler_free_buffer(nullptr);

    // Null context to destroy should not crash
    pd_compiler_destroy(nullptr);

    pd_compiler_destroy(ctx);
}

TEST(CompilerFfiTest, CompilationAndBufferOwnership) {
    pd_compiler_ctx_t* ctx = nullptr;
    ASSERT_EQ(pd_compiler_create(&ctx), PD_OK);

    const char* source_code = "widget Root() { return Center(child: Text('Hello')); }";
    pd_buffer_t buf{nullptr, 0};

    pd_status_t status = pd_compiler_compile(
        ctx,
        reinterpret_cast<const uint8_t*>(source_code),
        std::strlen(source_code),
        &buf
    );

    ASSERT_EQ(status, PD_OK);
    ASSERT_NE(buf.data, nullptr);
    ASSERT_GT(buf.length, 8u);

    // Verify magic header "PDC1"
    EXPECT_EQ(std::memcmp(buf.data, "PDC1", 4), 0);

    // Deallocate through Rust export
    pd_compiler_free_buffer(&buf);
    EXPECT_EQ(buf.data, nullptr);
    EXPECT_EQ(buf.length, 0u);

    pd_compiler_destroy(ctx);
}

TEST(CompilerFfiTest, MalformedInputReturnsErrorWithoutUnwinding) {
    pd_compiler_ctx_t* ctx = nullptr;
    ASSERT_EQ(pd_compiler_create(&ctx), PD_OK);

    pd_buffer_t buf{nullptr, 0};

    // Empty source
    pd_status_t status = pd_compiler_compile(ctx, nullptr, 0, &buf);
    EXPECT_EQ(status, PD_COMPILER_ERROR);
    EXPECT_EQ(buf.data, nullptr);

    // Syntax error
    const char* bad_src = "syntax_error invalid tokens";
    status = pd_compiler_compile(
        ctx,
        reinterpret_cast<const uint8_t*>(bad_src),
        std::strlen(bad_src),
        &buf
    );
    EXPECT_EQ(status, PD_COMPILER_ERROR);
    EXPECT_EQ(buf.data, nullptr);

    pd_compiler_destroy(ctx);
}

TEST(CompilerFfiTest, PanicContainmentDoesNotCrashHost) {
    pd_compiler_ctx_t* ctx = nullptr;
    ASSERT_EQ(pd_compiler_create(&ctx), PD_OK);

    // Source designed to trigger an internal panic in the Rust implementation
    const char* panic_src = "trigger_panic intentional crash";
    pd_buffer_t buf{nullptr, 0};

    pd_status_t status = pd_compiler_compile(
        ctx,
        reinterpret_cast<const uint8_t*>(panic_src),
        std::strlen(panic_src),
        &buf
    );

    // Panic must be contained by catch_unwind and returned as PD_INTERNAL_ERROR
    EXPECT_EQ(status, PD_INTERNAL_ERROR);
    EXPECT_EQ(buf.data, nullptr);
    EXPECT_EQ(buf.length, 0u);

    pd_compiler_destroy(ctx);
}

TEST(CompilerFfiTest, CppRaiiCompilerWrapper) {
    auto compiler_res = Compiler::create();
    ASSERT_TRUE(compiler_res.has_value());
    Compiler compiler = std::move(compiler_res).value();

    auto bytecode_res = compiler.compile("widget App() { return View(); }");
    ASSERT_TRUE(bytecode_res.has_value());
    const auto& bytecode = bytecode_res.value();
    ASSERT_GT(bytecode.size(), 8u);
    EXPECT_EQ(std::memcmp(bytecode.data(), "PDC1", 4), 0);

    // Test error case via wrapper
    auto error_res = compiler.compile("syntax_error foo");
    EXPECT_FALSE(error_res.has_value());

    // Test panic containment via wrapper
    auto panic_res = compiler.compile("trigger_panic bar");
    EXPECT_FALSE(panic_res.has_value());
    EXPECT_EQ(panic_res.status().code(), StatusCode::Internal);
}

TEST(CompilerFfiTest, RepeatedLifecycleStressCycles) {
    constexpr int kCycles = 1000;
    const char* src = "widget Counter() { return Button(); }";

    for (int i = 0; i < kCycles; ++i) {
        pd_compiler_ctx_t* ctx = nullptr;
        ASSERT_EQ(pd_compiler_create(&ctx), PD_OK);

        pd_buffer_t buf{nullptr, 0};
        ASSERT_EQ(pd_compiler_compile(
            ctx,
            reinterpret_cast<const uint8_t*>(src),
            std::strlen(src),
            &buf
        ), PD_OK);

        pd_compiler_free_buffer(&buf);
        pd_compiler_destroy(ctx);
    }
}
