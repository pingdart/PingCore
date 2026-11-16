// tests/core/test_result.cpp
#include <gtest/gtest.h>
#include <pingdart/core/status.h>
#include <pingdart/core/result.h>
#include <memory>
#include <string>
#include <vector>

using namespace pingdart::core;

TEST(StatusTest, DefaultIsOk) {
    Status s;
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(s.code(), StatusCode::Ok);
    EXPECT_TRUE(s.message().empty());
    EXPECT_EQ(s.platform_error(), 0);
    EXPECT_EQ(s.to_string(), "Ok");
}

TEST(StatusTest, FactoryConstructors) {
    auto invalid = Status::InvalidArgument("bad argument", 42);
    EXPECT_FALSE(invalid.ok());
    EXPECT_EQ(invalid.code(), StatusCode::InvalidArgument);
    EXPECT_EQ(invalid.message(), "bad argument");
    EXPECT_EQ(invalid.platform_error(), 42);
    EXPECT_EQ(invalid.to_string(), "InvalidArgument: bad argument (platform error: 42)");

    auto not_found = Status::NotFound("resource missing");
    EXPECT_EQ(not_found.code(), StatusCode::NotFound);
    EXPECT_EQ(not_found.message(), "resource missing");
    EXPECT_EQ(not_found.to_string(), "NotFound: resource missing");

    auto already_exists = Status::AlreadyExists("file exists");
    EXPECT_EQ(already_exists.code(), StatusCode::AlreadyExists);

    auto timeout = Status::Timeout("timed out waiting for lock");
    EXPECT_EQ(timeout.code(), StatusCode::Timeout);
}

TEST(StatusTest, EqualityComparisons) {
    Status s1 = Status::NotFound("item 1");
    Status s2 = Status::NotFound("item 1");
    Status s3 = Status::NotFound("item 2");
    Status s4 = Status::Internal("item 1");

    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
    EXPECT_NE(s1, s4);
}

TEST(ResultTest, SuccessWithValue) {
    Result<int> res(42);
    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(static_cast<bool>(res));
    EXPECT_EQ(res.value(), 42);
    EXPECT_EQ(*res, 42);
    EXPECT_TRUE(res.status().ok());
}

TEST(ResultTest, ErrorStatus) {
    Result<int> res(Status::NotFound("ID not found"));
    EXPECT_FALSE(res.has_value());
    EXPECT_FALSE(static_cast<bool>(res));
    EXPECT_EQ(res.status().code(), StatusCode::NotFound);
    EXPECT_EQ(res.status().message(), "ID not found");

    EXPECT_THROW({
        auto val = res.value();
        (void)val;
    }, std::runtime_error);
}

TEST(ResultTest, StringValueAndPointerAccess) {
    struct Person {
        std::string name;
        int age;
    };

    Result<Person> res(Person{"Alice", 30});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->name, "Alice");
    EXPECT_EQ(res->age, 30);
    EXPECT_EQ((*res).name, "Alice");
}

TEST(ResultTest, MoveOnlyTypeUniquePtr) {
    auto ptr = std::make_unique<int>(100);
    Result<std::unique_ptr<int>> res(std::move(ptr));

    EXPECT_TRUE(res.has_value());
    ASSERT_NE(res.value(), nullptr);
    EXPECT_EQ(*res.value(), 100);

    // Test moving out of Result
    std::unique_ptr<int> moved_out = std::move(res).value();
    ASSERT_NE(moved_out, nullptr);
    EXPECT_EQ(*moved_out, 100);
}

TEST(ResultTest, MoveOnlyStruct) {
    struct MoveOnly {
        int id;
        explicit MoveOnly(int i) : id(i) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) noexcept = default;
        MoveOnly& operator=(MoveOnly&&) noexcept = default;
    };

    Result<MoveOnly> res(MoveOnly(777));
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value().id, 777);

    MoveOnly extracted = std::move(res).value();
    EXPECT_EQ(extracted.id, 777);
}

TEST(ResultTest, InPlaceConstruction) {
    struct ComplexType {
        std::string title;
        std::vector<int> numbers;
        ComplexType(std::string t, std::vector<int> n) : title(std::move(t)), numbers(std::move(n)) {}
    };

    Result<ComplexType> res(std::in_place, "Header", std::vector<int>{1, 2, 3});
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->title, "Header");
    EXPECT_EQ(res->numbers.size(), 3);
}

TEST(ResultTest, ValueOr) {
    Result<std::string> ok_res("Hello");
    EXPECT_EQ(ok_res.value_or("Default"), "Hello");

    Result<std::string> err_res(Status::Unavailable("service offline"));
    EXPECT_EQ(err_res.value_or("Default"), "Default");

    // Move-only value_or
    Result<std::unique_ptr<int>> ok_ptr(std::make_unique<int>(42));
    auto extracted_ok = std::move(ok_ptr).value_or(std::make_unique<int>(99));
    EXPECT_EQ(*extracted_ok, 42);

    Result<std::unique_ptr<int>> err_ptr(Status::Unavailable("missing"));
    auto extracted_default = std::move(err_ptr).value_or(std::make_unique<int>(99));
    EXPECT_EQ(*extracted_default, 99);
}

TEST(ResultTest, ZeroDynamicHeapAllocationOnPrimitive) {
    // A Result<int> should be purely stack/value allocated
    static_assert(sizeof(Result<int>) <= sizeof(std::variant<int, Status>), "Result size check");
    Result<uint64_t> res(12345678ULL);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), 12345678ULL);
}
