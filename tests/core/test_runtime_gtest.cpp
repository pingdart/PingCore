// tests/core/test_runtime_gtest.cpp
// Core Runtime Unit Tests using GoogleTest (ENG-102 verification)

#include <gtest/gtest.h>
#include <pingdart/core/runtime.h>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

using namespace pingdart::core;

class RuntimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        Runtime::shutdown();
    }

    void TearDown() override {
        Runtime::shutdown();
    }
};

TEST_F(RuntimeTest, InitialStateIsNotInitialized) {
    EXPECT_FALSE(Runtime::is_initialized());
}

TEST_F(RuntimeTest, InitializeSucceedsAndSetsFlag) {
    Status s = Runtime::initialize();
    EXPECT_TRUE(s.ok());
    EXPECT_TRUE(Runtime::is_initialized());
}

TEST_F(RuntimeTest, DuplicateInitializeFailsWithAlreadyExists) {
    Status s1 = Runtime::initialize();
    EXPECT_TRUE(s1.ok());

    Status s2 = Runtime::initialize();
    EXPECT_FALSE(s2.ok());
    EXPECT_EQ(s2.code(), StatusCode::AlreadyExists);
    EXPECT_EQ(s2.message(), "Runtime already initialized");
}

TEST_F(RuntimeTest, ShutdownClearsInitializedFlag) {
    EXPECT_TRUE(Runtime::initialize().ok());
    EXPECT_TRUE(Runtime::is_initialized());

    Runtime::shutdown();
    EXPECT_FALSE(Runtime::is_initialized());
}

TEST_F(RuntimeTest, VersionStringIsNonEmpty) {
    std::string_view ver = Runtime::version();
    EXPECT_FALSE(ver.empty());
    EXPECT_EQ(ver, "0.1.0-alpha");
}

TEST(ResultTest, ValueHoldsExpectedData) {
    Result<int> res(1234);
    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(static_cast<bool>(res));
    EXPECT_EQ(res.value(), 1234);
    EXPECT_TRUE(res.status().ok());
}

TEST(ResultTest, StatusHoldsExpectedError) {
    Status err(StatusCode::PermissionDenied, "Access denied to resource", 13);
    Result<std::string> res(err);

    EXPECT_FALSE(res.has_value());
    EXPECT_FALSE(static_cast<bool>(res));
    EXPECT_EQ(res.status().code(), StatusCode::PermissionDenied);
    EXPECT_EQ(res.status().message(), "Access denied to resource");
    EXPECT_EQ(res.status().platform_error(), 13);

    EXPECT_THROW({
        auto val = res.value();
        (void)val;
    }, std::runtime_error);
}
