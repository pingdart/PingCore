// include/pingcore/socket_compat.h
// Portable Socket Compatibility Layer for Cross-Platform Desktop & POSIX
#pragma once

#if defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif
#pragma warning(disable: 4996)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")

using ssize_t = intptr_t;

#ifndef AF_UNIX
#define AF_UNIX 1
#endif

inline int close_socket(int s) {
    return ::closesocket(static_cast<SOCKET>(s));
}

inline int portable_socketpair(int fds[2]) {
    static bool winsock_inited = false;
    if (!winsock_inited) {
        WSADATA wsa;
        int err = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (err != 0) {
            fprintf(stderr, "WSAStartup failed: %d\n", err);
            return -1;
        }
        winsock_inited = true;
    }
    SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        fprintf(stderr, "socket listener failed: %d\n", WSAGetLastError());
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        ::closesocket(listener);
        return -1;
    }
    int len = sizeof(addr);
    if (::getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &len) == SOCKET_ERROR) {
        fprintf(stderr, "getsockname failed: %d\n", WSAGetLastError());
        ::closesocket(listener);
        return -1;
    }
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::listen(listener, 1) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        ::closesocket(listener);
        return -1;
    }

    SOCKET client = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) {
        fprintf(stderr, "socket client failed: %d\n", WSAGetLastError());
        ::closesocket(listener);
        return -1;
    }
    if (::connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "connect failed: %d\n", WSAGetLastError());
        ::closesocket(listener);
        ::closesocket(client);
        return -1;
    }

    SOCKET server = ::accept(listener, nullptr, nullptr);
    ::closesocket(listener);
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
        ::closesocket(client);
        return -1;
    }

    fds[0] = static_cast<int>(client);
    fds[1] = static_cast<int>(server);
    return 0;
}

inline int socketpair(int family, int type, int proto, int fds[2]) {
    (void)family; (void)type; (void)proto;
    return portable_socketpair(fds);
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

inline int close_socket(int s) {
    return ::close(s);
}

inline int portable_socketpair(int fds[2]) {
    return ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
}
#endif
