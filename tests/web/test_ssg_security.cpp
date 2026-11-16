#include <gtest/gtest.h>
#include <pingdart/web/static_route.h>
#include <pingdart/web/url_security.h>
#include "../../src/web/ssg/static_generator.h"

using namespace pingdart::web;

TEST(SsgSecurityTest, SafeRouteParameterValidation) {
    EXPECT_TRUE(UrlSecurity::is_safe_route_parameter("jane-smith"));
    EXPECT_TRUE(UrlSecurity::is_safe_route_parameter("dr_specialist_12"));
    EXPECT_TRUE(UrlSecurity::is_safe_route_parameter("dr.smith"));

    // Traversal and escape attempts
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("."));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter(".."));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("../escape"));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("escape/nested"));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("escape\\backslash"));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("name with spaces"));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter("<script>"));
    EXPECT_FALSE(UrlSecurity::is_safe_route_parameter(""));
}

TEST(SsgSecurityTest, RejectRoutePathTraversal) {
    StaticGenerator ssg;

    ssg.add_route(StaticRoute{
        .path = "/doctors/../../etc/passwd"
    });

    auto res = ssg.build();
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.status().code(), pingdart::core::StatusCode::InvalidArgument);
    EXPECT_NE(res.status().message().find("SecurityViolation"), std::string::npos);
}

TEST(SsgSecurityTest, RejectEncodedPathTraversal) {
    StaticGenerator ssg;

    ssg.add_route(StaticRoute{
        .path = "/doctors/%2e%2e%2fescaped"
    });

    auto res = ssg.build();
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.status().code(), pingdart::core::StatusCode::InvalidArgument);
}

TEST(SsgSecurityTest, DetectDuplicateOutputCollisions) {
    StaticGenerator ssg;

    // Both routes resolve to doctors/index.html
    ssg.add_route(StaticRoute{.path = "/doctors"});
    ssg.add_route(StaticRoute{.path = "/doctors/"});

    auto res = ssg.build();
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.status().code(), pingdart::core::StatusCode::AlreadyExists);
    EXPECT_NE(res.status().message().find("Duplicate output path"), std::string::npos);
}
