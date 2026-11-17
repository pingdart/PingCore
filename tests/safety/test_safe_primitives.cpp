// tests/safety/test_safe_primitives.cpp
// PKG-115: Unit Tests for Memory Safety Primitives
#include <gtest/gtest.h>
#include <pingdart/safety/safe_buffer.h>
#include <pingdart/safety/checked_arithmetic.h>
#include <pingdart/safety/non_null.h>
#include <pingdart/safety/ownership.h>
#include <pingdart/safety/secure_memory.h>

using namespace pingdart::safety;

// ============================================================================
// 1. SafeSpan & SafeBuffer Bounds Checking
// ============================================================================

TEST(SafetyPrimitivesTest, SafeSpanBoundsEnforcement) {
    int arr[] = {10, 20, 30, 40, 50};
    SafeSpan<int> span(arr, 5);

    EXPECT_EQ(span.size(), 5u);
    EXPECT_FALSE(span.empty());
    EXPECT_EQ(span[0], 10);
    EXPECT_EQ(span.at(4), 50);
    EXPECT_EQ(span.front(), 10);
    EXPECT_EQ(span.back(), 50);

    // Out of bounds checks
    EXPECT_THROW(span.at(5), std::out_of_range);
    EXPECT_THROW(span[10], std::out_of_range);

    // Subspan bounds
    auto sub = span.subspan(1, 3);
    EXPECT_EQ(sub.size(), 3u);
    EXPECT_EQ(sub[0], 20);
    EXPECT_EQ(sub[2], 40);
    EXPECT_THROW(sub.at(3), std::out_of_range);
    EXPECT_THROW(span.subspan(6, 1), std::out_of_range);
}

TEST(SafetyPrimitivesTest, SafeBufferOperations) {
    SafeBuffer<uint32_t> buf(4, 0xAA);
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf.size_bytes(), 16u);
    EXPECT_EQ(buf[0], 0xAAu);

    EXPECT_THROW(buf.at(4), std::out_of_range);

    buf.append(0xBB);
    EXPECT_EQ(buf.size(), 5u);
    EXPECT_EQ(buf.at(4), 0xBBu);

    auto span = buf.as_span();
    EXPECT_EQ(span.size(), 5u);
    EXPECT_EQ(span[4], 0xBBu);
}

// ============================================================================
// 2. Overflow-Checked Arithmetic
// ============================================================================

TEST(SafetyPrimitivesTest, CheckedAddition) {
    auto ok_res = checked_add(100, 200);
    ASSERT_TRUE(ok_res.has_value());
    EXPECT_EQ(*ok_res, 300);

    // Unsigned overflow
    auto u_overflow = checked_add(std::numeric_limits<uint64_t>::max(), uint64_t{1});
    EXPECT_FALSE(u_overflow.has_value());

    // Signed overflow
    auto s_overflow = checked_add(std::numeric_limits<int32_t>::max(), int32_t{1});
    EXPECT_FALSE(s_overflow.has_value());

    // Signed underflow
    auto s_underflow = checked_add(std::numeric_limits<int32_t>::min(), int32_t{-1});
    EXPECT_FALSE(s_underflow.has_value());
}

TEST(SafetyPrimitivesTest, CheckedSubtraction) {
    auto ok_res = checked_sub(500, 200);
    ASSERT_TRUE(ok_res.has_value());
    EXPECT_EQ(*ok_res, 300);

    // Unsigned underflow
    auto u_underflow = checked_sub(uint32_t{10}, uint32_t{20});
    EXPECT_FALSE(u_underflow.has_value());

    // Signed underflow
    auto s_underflow = checked_sub(std::numeric_limits<int32_t>::min(), int32_t{1});
    EXPECT_FALSE(s_underflow.has_value());
}

TEST(SafetyPrimitivesTest, CheckedMultiplicationAndDivision) {
    auto mul_ok = checked_mul(25, 4);
    ASSERT_TRUE(mul_ok.has_value());
    EXPECT_EQ(*mul_ok, 100);

    // Multiplication overflow
    auto mul_overflow = checked_mul(std::numeric_limits<uint32_t>::max(), uint32_t{2});
    EXPECT_FALSE(mul_overflow.has_value());

    // Division by zero
    auto div_zero = checked_div(100, 0);
    EXPECT_FALSE(div_zero.has_value());

    // Signed division overflow (INT_MIN / -1)
    auto div_overflow = checked_div(std::numeric_limits<int32_t>::min(), -1);
    EXPECT_FALSE(div_overflow.has_value());
}

TEST(SafetyPrimitivesTest, CheckedTypeWrapper) {
    Checked<int32_t> a = 100;
    Checked<int32_t> b = 200;
    Checked<int32_t> c = a + b;
    EXPECT_EQ(c.get(), 300);

    Checked<int32_t> max_val = std::numeric_limits<int32_t>::max();
    EXPECT_THROW([[maybe_unused]] auto x = max_val + Checked<int32_t>(1), std::overflow_error);
    EXPECT_THROW([[maybe_unused]] auto y = a / Checked<int32_t>(0), std::overflow_error);
}

// ============================================================================
// 3. Non-Null Pointers & Ownership Wrappers
// ============================================================================

TEST(SafetyPrimitivesTest, NonNullPointerInvariants) {
    int value = 42;
    NonNull<int*> ptr(&value);
    EXPECT_EQ(*ptr, 42);
    *ptr = 100;
    EXPECT_EQ(value, 100);

    // Null pointer rejection
    int* null_raw = nullptr;
    EXPECT_THROW([[maybe_unused]] auto p = NonNull<int*>(null_raw), std::invalid_argument);

    // Non-null smart pointer
    auto nn_smart = make_non_null_unique<int>(999);
    EXPECT_EQ(*nn_smart, 999);

    std::unique_ptr<int> null_smart;
    EXPECT_THROW(NonNullPtr<std::unique_ptr<int>>(std::move(null_smart)), std::invalid_argument);
}

TEST(SafetyPrimitivesTest, UniqueRefAndBorrowing) {
    auto uref = UniqueRef<std::string>::make("PingDart Safety Core");
    EXPECT_EQ(*uref, "PingDart Safety Core");
    EXPECT_EQ(uref->size(), 20u);

    // Borrow
    BorrowedRef<std::string> borrow(uref);
    EXPECT_EQ(borrow->size(), 20u);
    EXPECT_EQ(*borrow, "PingDart Safety Core");

    // Move ownership
    UniqueRef<std::string> moved = std::move(uref);
    EXPECT_EQ(*moved, "PingDart Safety Core");
}

// ============================================================================
// 4. Secure Memory Scrubbing
// ============================================================================

TEST(SafetyPrimitivesTest, SecureMemoryZeroing) {
    const char secret[] = "SuperSecretPassword1234!";
    SecureBuffer sec(secret, sizeof(secret));

    EXPECT_EQ(sec.size(), sizeof(secret));
    EXPECT_FALSE(sec.empty());
    EXPECT_EQ(std::memcmp(sec.data(), secret, sizeof(secret)), 0);

    // Lock memory pages (if OS allows)
    [[maybe_unused]] auto lock_res = sec.lock_memory();

    // Wipe memory
    sec.wipe();
    EXPECT_EQ(sec.size(), 0u);
    EXPECT_EQ(sec.data(), nullptr);
}
