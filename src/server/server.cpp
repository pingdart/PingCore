// src/server/server.cpp
#include <pingcore/server.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

namespace pingcore {

pingdart::core::Status Server::listen(uint16_t port, bool non_blocking) {
    if (is_running_.load()) {
        return pingdart::core::Status(pingdart::core::StatusCode::AlreadyExists, "Server already running");
    }

    port_ = port;
    is_running_.store(true);

    if (non_blocking) {
        // Non-blocking in-memory or background mode for testing
        return pingdart::core::Status::Ok();
    }

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        is_running_.store(false);
        return pingdart::core::Status(pingdart::core::StatusCode::Internal, "Failed to create server socket");
    }

    int opt = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(server_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        ::close(server_fd);
        is_running_.store(false);
        return pingdart::core::Status(pingdart::core::StatusCode::Internal, "Failed to bind to port " + std::to_string(port));
    }

    struct sockaddr_in bound_addr{};
    socklen_t bound_len = sizeof(bound_addr);
    if (::getsockname(server_fd, reinterpret_cast<struct sockaddr*>(&bound_addr), &bound_len) == 0) {
        port_ = ntohs(bound_addr.sin_port);
    }

    if (::listen(server_fd, 128) < 0) {
        ::close(server_fd);
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

            int activity = ::select(server_fd + 1, &read_fds, nullptr, nullptr, &tv);
            if (activity > 0 && FD_ISSET(server_fd, &read_fds)) {
                struct sockaddr_in client_addr{};
                socklen_t addrlen = sizeof(client_addr);
                int client_fd = ::accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen);
                if (client_fd >= 0) {
#ifdef SO_NOSIGPIPE
                    int nosigpipe = 1;
                    ::setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
                    if (!can_accept_connection()) {
                        std::string resp503 = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 19\r\nConnection: close\r\n\r\nService Unavailable";
                        (void)::write(client_fd, resp503.data(), resp503.size());
                        ::close(client_fd);
                        continue;
                    }

                    uint64_t conn_id = tracker_.add_connection("client");

                    // PKG-127: Set socket read timeout for slowloris prevention
                    struct timeval timeout{};
                    timeout.tv_sec = static_cast<long>(server_config_.client_header_timeout.count() / 1000);
                    timeout.tv_usec = static_cast<int>((server_config_.client_header_timeout.count() % 1000) * 1000);
                    ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

                    std::string stream_buffer;
                    char buffer[8192]{};
                    ssize_t bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1);
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
                                (void)::write(client_fd, wire_resp.data(), wire_resp.size());
                            }
                        } else if (pipelined.incomplete) {
                            std::string resp408 = "HTTP/1.1 408 Request Timeout\r\nContent-Length: 15\r\nConnection: close\r\n\r\nRequest Timeout";
                            (void)::write(client_fd, resp408.data(), resp408.size());
                        } else {
                            Response res;
                            res.status(400).send("Bad Request: " + pipelined.error);
                            std::string wire_resp = res.to_http_wire_format();
                            (void)::write(client_fd, wire_resp.data(), wire_resp.size());
                        }
                    }
                    tracker_.remove_connection(conn_id);
                    ::close(client_fd);
                }
            }
        }
        ::close(server_fd);
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
