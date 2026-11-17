// tests/server/test_pingcore_server.cpp
// Comprehensive test suite for PingCore Backend API SDK (PKG-119)

#include <gtest/gtest.h>
#include <pingcore/server.h>
#include <thread>
#include <chrono>

using namespace pingcore;

TEST(PingCoreServerTest, BasicRequestProperties) {
    Request req("GET", "/api/v1/health", "{\"client\":\"pd\"}");
    req.set_header("Authorization", "Bearer secret_token");
    req.set_param("query", "status");

    EXPECT_EQ(req.method(), "GET");
    EXPECT_EQ(req.path(), "/api/v1/health");
    EXPECT_EQ(req.body(), "{\"client\":\"pd\"}");
    EXPECT_EQ(req.header("Authorization"), "Bearer secret_token");
    EXPECT_EQ(req.header("NonExistent"), "");
    EXPECT_EQ(req.param("query"), "status");
    EXPECT_EQ(req.param("unknown"), "");
}

TEST(PingCoreServerTest, ResponseWireFormatAndHeaders) {
    Response res;
    res.status(200)
       .header("X-Powered-By", "PingCore")
       .json("{\"status\":\"ok\"}");

    EXPECT_EQ(res.status_code(), 200);
    EXPECT_EQ(res.body(), "{\"status\":\"ok\"}");
    EXPECT_EQ(res.headers().at("Content-Type"), "application/json; charset=utf-8");
    EXPECT_EQ(res.headers().at("X-Powered-By"), "PingCore");

    std::string wire = res.to_http_wire_format();
    EXPECT_NE(wire.find("HTTP/1.1 200 OK\r\n"), std::string::npos);
    EXPECT_NE(wire.find("Content-Type: application/json; charset=utf-8\r\n"), std::string::npos);
    EXPECT_NE(wire.find("Content-Length: 15\r\n"), std::string::npos);
    EXPECT_NE(wire.find("\r\n\r\n{\"status\":\"ok\"}"), std::string::npos);
}

TEST(PingCoreServerTest, StatusCodesWireFormatting) {
    Response r201; r201.status(201).send("Created");
    EXPECT_NE(r201.to_http_wire_format().find("HTTP/1.1 201 Created\r\n"), std::string::npos);

    Response r204; r204.status(204);
    EXPECT_NE(r204.to_http_wire_format().find("HTTP/1.1 204 No Content\r\n"), std::string::npos);

    Response r400; r400.status(400).send("Bad");
    EXPECT_NE(r400.to_http_wire_format().find("HTTP/1.1 400 Bad Request\r\n"), std::string::npos);

    Response r401; r401.status(401).send("Auth");
    EXPECT_NE(r401.to_http_wire_format().find("HTTP/1.1 401 Unauthorized\r\n"), std::string::npos);

    Response r403; r403.status(403).send("Forbidden");
    EXPECT_NE(r403.to_http_wire_format().find("HTTP/1.1 403 Forbidden\r\n"), std::string::npos);

    Response r404; r404.status(404).send("Not Found");
    EXPECT_NE(r404.to_http_wire_format().find("HTTP/1.1 404 Not Found\r\n"), std::string::npos);

    Response r500; r500.status(500).send("Error");
    EXPECT_NE(r500.to_http_wire_format().find("HTTP/1.1 500 Internal Server Error\r\n"), std::string::npos);
}

TEST(PingCoreServerTest, HttpMethodRouting) {
    Server server;

    server.get("/items", [](const Request&, Response& res) {
        res.status(200).json("[{\"id\":1}]");
    });

    server.post("/items", [](const Request& req, Response& res) {
        res.status(201).json("{\"created\":true,\"data\":\"" + req.body() + "\"}");
    });

    server.put("/items", [](const Request&, Response& res) {
        res.status(200).json("{\"updated\":true}");
    });

    server.delete_("/items", [](const Request&, Response& res) {
        res.status(204);
    });

    // Test GET
    auto get_res = server.handle_request(Request("GET", "/items"));
    EXPECT_EQ(get_res.status_code(), 200);
    EXPECT_EQ(get_res.body(), "[{\"id\":1}]");

    // Test POST
    auto post_res = server.handle_request(Request("POST", "/items", "new_item"));
    EXPECT_EQ(post_res.status_code(), 201);
    EXPECT_EQ(post_res.body(), "{\"created\":true,\"data\":\"new_item\"}");

    // Test PUT
    auto put_res = server.handle_request(Request("PUT", "/items"));
    EXPECT_EQ(put_res.status_code(), 200);
    EXPECT_EQ(put_res.body(), "{\"updated\":true}");

    // Test DELETE
    auto del_res = server.handle_request(Request("DELETE", "/items"));
    EXPECT_EQ(del_res.status_code(), 204);

    // Test Unmatched Route (404)
    auto not_found = server.handle_request(Request("GET", "/unknown"));
    EXPECT_EQ(not_found.status_code(), 404);
    EXPECT_EQ(not_found.body(), "Not Found");
}

TEST(PingCoreServerTest, ParameterizedRouteExtraction) {
    Server server;

    server.get("/users/:userId/posts/:postId", [](const Request& req, Response& res) {
        std::string uid = req.param("userId");
        std::string pid = req.param("postId");
        res.status(200).json("{\"user\":\"" + uid + "\",\"post\":\"" + pid + "\"}");
    });

    auto res = server.handle_request(Request("GET", "/users/42/posts/108"));
    EXPECT_EQ(res.status_code(), 200);
    EXPECT_EQ(res.body(), "{\"user\":\"42\",\"post\":\"108\"}");

    // Mismatched segment count should 404
    auto mismatch = server.handle_request(Request("GET", "/users/42/posts"));
    EXPECT_EQ(mismatch.status_code(), 404);
}

TEST(PingCoreServerTest, MiddlewarePipelineChaining) {
    Server server;
    std::vector<std::string> log;

    server.use([&log](Request&, Response&, Next next) {
        log.push_back("mw1_before");
        next();
        log.push_back("mw1_after");
    });

    server.use([&log](Request&, Response&, Next next) {
        log.push_back("mw2_before");
        next();
        log.push_back("mw2_after");
    });

    server.get("/test", [&log](const Request&, Response& res) {
        log.push_back("route_handler");
        res.status(200).send("OK");
    });

    auto res = server.handle_request(Request("GET", "/test"));
    EXPECT_EQ(res.status_code(), 200);
    EXPECT_EQ(res.body(), "OK");

    std::vector<std::string> expected = {
        "mw1_before",
        "mw2_before",
        "route_handler",
        "mw2_after",
        "mw1_after"
    };
    EXPECT_EQ(log, expected);
}

TEST(PingCoreServerTest, MiddlewareShortCircuitingAuth) {
    Server server;

    server.use([](Request& req, Response& res, Next next) {
        if (req.header("Authorization") != "Bearer valid_key") {
            res.status(401).json("{\"error\":\"Unauthorized\"}");
            return; // Short-circuit, do not call next()
        }
        next();
    });

    server.get("/protected", [](const Request&, Response& res) {
        res.status(200).json("{\"data\":\"confidential\"}");
    });

    // 1. Rejected request
    Request unauthorized_req("GET", "/protected");
    auto unauth_res = server.handle_request(unauthorized_req);
    EXPECT_EQ(unauth_res.status_code(), 401);
    EXPECT_EQ(unauth_res.body(), "{\"error\":\"Unauthorized\"}");

    // 2. Allowed request
    Request authorized_req("GET", "/protected");
    authorized_req.set_header("Authorization", "Bearer valid_key");
    auto auth_res = server.handle_request(authorized_req);
    EXPECT_EQ(auth_res.status_code(), 200);
    EXPECT_EQ(auth_res.body(), "{\"data\":\"confidential\"}");
}

TEST(PingCoreServerTest, ServerLifecycleNonBlocking) {
    Server server;
    EXPECT_FALSE(server.is_running());

    auto status = server.listen(8080, true /* non-blocking */);
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(server.is_running());
    EXPECT_EQ(server.port(), 8080);

    // Duplicate listen should fail
    auto dup = server.listen(8080, true);
    EXPECT_FALSE(dup.ok());

    server.stop();
    EXPECT_FALSE(server.is_running());
}
