// src/server/server.cpp
#include <pingcore/server.h>
#include <iostream>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
inline int sock_close(SOCKET s) { return ::closesocket(s); }
inline int sock_read(SOCKET s, char* buf, int len) { return ::recv(s, buf, len, 0); }
inline int sock_write(SOCKET s, const char* buf, int len) { return ::send(s, buf, len, 0); }
inline int sock_setopt(SOCKET s, int lvl, int optname, const void* optval, int optlen) {
    return ::setsockopt(s, lvl, optname, reinterpret_cast<const char*>(optval), optlen);
}
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
using socket_t = int;
inline int sock_close(int s) { return ::close(s); }
inline ssize_t sock_read(int s, char* buf, size_t len) { return ::read(s, buf, len); }
inline ssize_t sock_write(int s, const char* buf, size_t len) { return ::write(s, buf, len); }
inline int sock_setopt(int s, int lvl, int optname, const void* optval, socklen_t optlen) {
    return ::setsockopt(s, lvl, optname, optval, optlen);
}
#endif

namespace pingcore {

pingdart::core::Status Server::listen(uint16_t port, bool non_blocking, std::string bind_address) {
    if (is_running_.load()) {
        return pingdart::core::Status(pingdart::core::StatusCode::AlreadyExists, "Server already running");
    }

    port_ = port;
    is_running_.store(true);

    if (non_blocking) {
        // Non-blocking in-memory or background mode for testing
        return pingdart::core::Status::Ok();
    }

#if defined(_WIN32)
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    socket_t server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == (socket_t)-1) {
        is_running_.store(false);
        return pingdart::core::Status(pingdart::core::StatusCode::Internal, "Failed to create server socket");
    }

    int opt = 1;
    sock_setopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    if (bind_address == "0.0.0.0") {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
#if defined(_WIN32)
        const int address_valid = ::InetPtonA(AF_INET, bind_address.c_str(), &address.sin_addr);
#else
        const int address_valid = ::inet_pton(AF_INET, bind_address.c_str(), &address.sin_addr);
#endif
        if (address_valid != 1) {
            sock_close(server_fd);
            is_running_.store(false);
            return pingdart::core::Status(
                pingdart::core::StatusCode::InvalidArgument,
                "Invalid IPv4 bind address: " + bind_address);
        }
    }
    address.sin_port = htons(port);

    if (::bind(server_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        sock_close(server_fd);
        is_running_.store(false);
        return pingdart::core::Status(pingdart::core::StatusCode::Internal, "Failed to bind to port " + std::to_string(port));
    }

    struct sockaddr_in bound_addr{};
    socklen_t bound_len = sizeof(bound_addr);
    if (::getsockname(server_fd, reinterpret_cast<struct sockaddr*>(&bound_addr), &bound_len) == 0) {
        port_ = ntohs(bound_addr.sin_port);
    }

    if (::listen(server_fd, 128) < 0) {
        sock_close(server_fd);
        is_running_.store(false);
        return pingdart::core::Status(pingdart::core::StatusCode::Internal, "Failed to listen on socket");
    }

    listener_thread_ = std::make_unique<std::thread>([this, server_fd]() {
        while (is_running_.load()) {
            struct timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms select timeout for clean shutdown check
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);

            int activity = ::select(static_cast<int>(server_fd) + 1, &read_fds, nullptr, nullptr, &tv);
            if (activity > 0 && FD_ISSET(server_fd, &read_fds)) {
                struct sockaddr_in client_addr{};
                socklen_t addrlen = sizeof(client_addr);
                socket_t client_fd = ::accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen);
                if (client_fd != (socket_t)-1) {
#ifdef SO_NOSIGPIPE
                    int nosigpipe = 1;
                    sock_setopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
                    if (!can_accept_connection()) {
                        std::string resp503 = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 19\r\nConnection: close\r\n\r\nService Unavailable";
                        (void)sock_write(client_fd, resp503.data(), static_cast<int>(resp503.size()));
                        sock_close(client_fd);
                        continue;
                    }

                    uint64_t conn_id = tracker_.add_connection("client");

                    // PKG-127: Set socket read timeout for slowloris prevention
#if defined(_WIN32)
                    DWORD timeout_ms = static_cast<DWORD>(server_config_.client_header_timeout.count());
                    sock_setopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
#else
                    struct timeval timeout{};
                    timeout.tv_sec = static_cast<long>(server_config_.client_header_timeout.count() / 1000);
                    timeout.tv_usec = static_cast<int>((server_config_.client_header_timeout.count() % 1000) * 1000);
                    sock_setopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif

                    std::string stream_buffer;
                    char buffer[8192]{};
                    auto bytes_read = sock_read(client_fd, buffer, static_cast<int>(sizeof(buffer) - 1));
                    if (bytes_read > 0) {
                        tracker_.record_bytes_received(conn_id, static_cast<size_t>(bytes_read));
                        stream_buffer.append(buffer, static_cast<size_t>(bytes_read));

                        auto pipelined = parse_http_requests_pipelined(stream_buffer);
                        if (!pipelined.requests.empty()) {
                            for (const auto& parsed : pipelined.requests) {
                                Response res;
                                if (!parsed.valid) {
                                    res.status(parsed.status_code > 0 ? parsed.status_code : 400).send(parsed.error);
                                } else {
                                    Request req = to_request(parsed);
                                    res = handle_request(std::move(req));
                                }
                                std::string wire_resp = res.to_http_wire_format();
                                tracker_.record_bytes_sent(conn_id, wire_resp.size());
                                (void)sock_write(client_fd, wire_resp.data(), static_cast<int>(wire_resp.size()));
                            }
                        } else if (pipelined.incomplete) {
                            std::string resp408 = "HTTP/1.1 408 Request Timeout\r\nContent-Length: 15\r\nConnection: close\r\n\r\nRequest Timeout";
                            (void)sock_write(client_fd, resp408.data(), static_cast<int>(resp408.size()));
                        } else {
                            Response res;
                            res.status(400).send("Bad Request: " + pipelined.error);
                            std::string wire_resp = res.to_http_wire_format();
                            (void)sock_write(client_fd, wire_resp.data(), static_cast<int>(wire_resp.size()));
                        }
                    }
                    tracker_.remove_connection(conn_id);
                    sock_close(client_fd);
                }
            }
        }
        sock_close(server_fd);
    });

    return pingdart::core::Status::Ok();
}

void Server::stop() {
    if (is_running_.exchange(false)) {
        if (listener_thread_ && listener_thread_->joinable()) {
            listener_thread_->join();
            listener_thread_.reset();
        }
    }
}

} // namespace pingcore
