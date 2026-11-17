// tests/router/test_nextjs_router.cpp
// Comprehensive test suite for PingCore Next.js-Style File-Based Router & UI Components (PKG-119)

#include <gtest/gtest.h>
#include <pingcore/router.h>
#include <pingcore/ui.h>
#include <pingcore/server.h>
#include <filesystem>
#include <fstream>

using namespace pingcore;

class NextJsRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        router().clear();
    }
};

TEST_F(NextJsRouterTest, BasicStaticRouteNavigation) {
    bool home_rendered = false;
    bool login_rendered = false;

    router().register_page("/", [&]() -> Widget {
        home_rendered = true;
        return Center(Text("Home Page"));
    });

    router().register_page("/login", [&]() -> Widget {
        login_rendered = true;
        return Center(Text("Login Page"));
    });

    // 1. Initial route
    router().navigate("/");
    EXPECT_EQ(router().current_path(), "/");
    auto home_widget = router().render_current_page();
    EXPECT_NE(home_widget, nullptr);
    EXPECT_TRUE(home_rendered);

    // 2. Navigate to /login
    router().navigate("/login");
    EXPECT_EQ(router().current_path(), "/login");
    auto login_widget = router().render_current_page();
    EXPECT_NE(login_widget, nullptr);
    EXPECT_TRUE(login_rendered);
}

TEST_F(NextJsRouterTest, DynamicRouteParameterExtraction) {
    router().register_page("/users/[id]", []() -> Widget {
        std::string uid = router().param("id");
        return Center(Text("User: " + uid));
    });

    router().navigate("/users/42");
    EXPECT_EQ(router().current_path(), "/users/42");
    EXPECT_EQ(router().param("id"), "42");

    auto widget = router().render_current_page();
    ASSERT_NE(widget, nullptr);

    // Navigate to another dynamic user
    router().navigate("/users/admin-alice");
    EXPECT_EQ(router().current_path(), "/users/admin-alice");
    EXPECT_EQ(router().param("id"), "admin-alice");
}

TEST_F(NextJsRouterTest, NestedLayoutCompositionOrder) {
    std::vector<std::string> layout_trace;

    // Root layout
    router().register_layout("/", [&](Widget child) -> Widget {
        layout_trace.push_back("root_layout");
        return Container().padding(16.0f).child(std::move(child));
    });

    // Subtree dashboard layout
    router().register_layout("/dashboard", [&](Widget child) -> Widget {
        layout_trace.push_back("dashboard_layout");
        return Container().padding(8.0f).child(std::move(child));
    });

    router().register_page("/dashboard", [&]() -> Widget {
        layout_trace.push_back("dashboard_page");
        return Text("Dashboard Content");
    });

    router().navigate("/dashboard");
    auto widget = router().render_current_page();
    ASSERT_NE(widget, nullptr);

    // Expected order: innermost page rendered, then wrapped by dashboard_layout, then root_layout
    std::vector<std::string> expected = {
        "dashboard_page",
        "dashboard_layout",
        "root_layout"
    };
    EXPECT_EQ(layout_trace, expected);
}

TEST_F(NextJsRouterTest, NotFoundFallbackPage) {
    bool not_found_rendered = false;

    router().register_not_found([&]() -> Widget {
        not_found_rendered = true;
        return Center(Text("Custom 404"));
    });

    router().navigate("/this/does/not/exist");
    auto widget = router().render_current_page();
    EXPECT_NE(widget, nullptr);
    EXPECT_TRUE(not_found_rendered);
}

TEST_F(NextJsRouterTest, RouteScannerAppDirectoryDiscovery) {
    std::filesystem::path temp_app = "dist/temp_nextjs_app/app";
    std::filesystem::create_directories(temp_app / "login");
    std::filesystem::create_directories(temp_app / "dashboard");
    std::filesystem::create_directories(temp_app / "users" / "[id]");
    std::filesystem::create_directories(temp_app / "api" / "users");

    // Create dummy files
    std::ofstream(temp_app / "layout.cpp") << "// root layout\n";
    std::ofstream(temp_app / "page.cpp") << "// root page\n";
    std::ofstream(temp_app / "loading.cpp") << "// loading\n";
    std::ofstream(temp_app / "not_found.cpp") << "// 404\n";
    std::ofstream(temp_app / "login" / "page.cpp") << "// login page\n";
    std::ofstream(temp_app / "dashboard" / "page.cpp") << "// dashboard page\n";
    std::ofstream(temp_app / "users" / "[id]" / "page.cpp") << "// user id page\n";
    std::ofstream(temp_app / "api" / "users" / "route.cpp") << "// api route\n";

    auto scan_res = RouteScanner::scan(temp_app);

    // Verify discovered pages
    EXPECT_EQ(scan_res.pages.size(), 4u);
    std::vector<std::string> discovered_routes;
    for (const auto& p : scan_res.pages) {
        discovered_routes.push_back(p.route_path);
    }
    EXPECT_NE(std::find(discovered_routes.begin(), discovered_routes.end(), "/"), discovered_routes.end());
    EXPECT_NE(std::find(discovered_routes.begin(), discovered_routes.end(), "/login"), discovered_routes.end());
    EXPECT_NE(std::find(discovered_routes.begin(), discovered_routes.end(), "/dashboard"), discovered_routes.end());
    EXPECT_NE(std::find(discovered_routes.begin(), discovered_routes.end(), "/users/:id"), discovered_routes.end());

    // Verify discovered layout
    EXPECT_EQ(scan_res.layouts.size(), 1u);
    EXPECT_EQ(scan_res.layouts[0].route_prefix, "/");

    // Verify discovered API routes
    EXPECT_EQ(scan_res.api_routes.size(), 1u);
    EXPECT_EQ(scan_res.api_routes[0].api_path, "/api/users");

    // Clean up
    std::filesystem::remove_all("dist/temp_nextjs_app");
}

TEST_F(NextJsRouterTest, ApiRouteHandlerWithInitializerListJson) {
    // Exactly matches the user's recommended API syntax:
    auto handler = [](Request& req) -> Response {
        (void)req;
        return Response::json({
            {"success", true},
            {"message", "PingCore API working"}
        });
    };

    Request req("GET", "/api/users");
    Response res = handler(req);

    EXPECT_EQ(res.status_code(), 200);
    EXPECT_EQ(res.headers().at("Content-Type"), "application/json; charset=utf-8");
    EXPECT_NE(res.body().find("\"success\":true"), std::string::npos);
    EXPECT_NE(res.body().find("\"message\":\"PingCore API working\""), std::string::npos);
}

TEST_F(NextJsRouterTest, DeveloperCodeExampleCompilationAndNavigation) {
    // Test the exact code snippet requested by the user:
    auto Page = []() -> Widget {
        return Center(
            Column({
                Text("Login").fontSize(28.0f).bold(),
                TextField("Email"),
                PasswordField("Password"),
                Button("Sign In")
                    .onClick([] {
                        navigate("/dashboard");
                    })
            }).spacing(12.0f)
        );
    };

    Widget w = Page();
    ASSERT_NE(w, nullptr);

    // Register page and navigate
    router().register_page("/login", Page);
    router().navigate("/login");
    EXPECT_EQ(router().current_path(), "/login");

    // Verify navigate updates current path
    navigate("/dashboard");
    EXPECT_EQ(router().current_path(), "/dashboard");
}
