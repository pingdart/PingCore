// include/pingcore/https_server.h
// Production-Grade HTTPS Server & TLS 1.2/1.3 Termination Engine (PKG-128)
#pragma once

#include <pingcore/server.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef PINGCORE_ENABLE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#endif

namespace pingcore {

struct HttpsResponse {
    int status_code{0};
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::string error;
    bool success{false};
};

class TlsCertificateConfig {
public:
    static bool generate_self_signed(std::string& out_cert_pem, std::string& out_key_pem,
                                     const std::string& cn = "localhost") {
#ifdef PINGCORE_ENABLE_OPENSSL
        EVP_PKEY* pkey = EVP_RSA_gen(2048);
        if (!pkey) return false;

        X509* x509 = X509_new();
        if (!x509) {
            EVP_PKEY_free(pkey);
            return false;
        }

        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1 year
        X509_set_pubkey(x509, pkey);

        X509_NAME* name = const_cast<X509_NAME*>(X509_get_subject_name(x509));
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)"US", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)"PingDart Framework", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)cn.c_str(), -1, -1, 0);
        X509_set_issuer_name(x509, name);

        if (X509_sign(x509, pkey, EVP_sha256()) <= 0) {
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }

        BIO* cert_bio = BIO_new(BIO_s_mem());
        PEM_write_bio_X509(cert_bio, x509);
        char* cert_data = nullptr;
        long cert_len = BIO_get_mem_data(cert_bio, &cert_data);
        if (cert_data && cert_len > 0) {
            out_cert_pem = std::string(cert_data, static_cast<size_t>(cert_len));
        }
        BIO_free(cert_bio);

        BIO* key_bio = BIO_new(BIO_s_mem());
        PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
        char* key_data = nullptr;
        long key_len = BIO_get_mem_data(key_bio, &key_data);
        if (key_data && key_len > 0) {
            out_key_pem = std::string(key_data, static_cast<size_t>(key_len));
        }
        BIO_free(key_bio);

        X509_free(x509);
        EVP_PKEY_free(pkey);
        return !out_cert_pem.empty() && !out_key_pem.empty();
#else
        (void)out_cert_pem; (void)out_key_pem; (void)cn;
        return false;
#endif
    }
};

class HttpsServer : public Server {
public:
    HttpsServer() = default;

    ~HttpsServer() {
        stop_tls();
    }

    bool init_tls(const std::string& cert_pem = "", const std::string& key_pem = "") {
#ifdef PINGCORE_ENABLE_OPENSSL
        std::lock_guard<std::mutex> lock(tls_mutex_);
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        const SSL_METHOD* method = TLS_server_method();
        ssl_ctx_ = SSL_CTX_new(method);
        if (!ssl_ctx_) {
            tls_error_ = "Failed to create SSL_CTX";
            return false;
        }

        // Set modern TLS protocols: TLS 1.2 and TLS 1.3
        SSL_CTX_set_min_proto_version(ssl_ctx_, TLS1_2_VERSION);

        std::string cert = cert_pem;
        std::string key = key_pem;

        if (cert.empty() || key.empty()) {
            if (!TlsCertificateConfig::generate_self_signed(cert, key, "localhost")) {
                tls_error_ = "Failed to generate self-signed TLS certificate";
                return false;
            }
        }

        active_cert_pem_ = cert;
        active_key_pem_ = key;

        BIO* cbio = BIO_new_mem_buf(cert.data(), static_cast<int>(cert.size()));
        X509* x509 = PEM_read_bio_X509(cbio, nullptr, nullptr, nullptr);
        BIO_free(cbio);

        if (!x509 || SSL_CTX_use_certificate(ssl_ctx_, x509) <= 0) {
            tls_error_ = "Failed to load TLS certificate into SSL_CTX";
            if (x509) X509_free(x509);
            return false;
        }
        X509_free(x509);

        BIO* kbio = BIO_new_mem_buf(key.data(), static_cast<int>(key.size()));
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(kbio, nullptr, nullptr, nullptr);
        BIO_free(kbio);

        if (!pkey || SSL_CTX_use_PrivateKey(ssl_ctx_, pkey) <= 0) {
            tls_error_ = "Failed to load TLS private key into SSL_CTX";
            if (pkey) EVP_PKEY_free(pkey);
            return false;
        }
        EVP_PKEY_free(pkey);

        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            tls_error_ = "TLS private key does not match certificate";
            return false;
        }

        tls_ready_ = true;
        return true;
#else
        (void)cert_pem; (void)key_pem;
        tls_error_ = "OpenSSL not enabled in this build";
        return false;
#endif
    }

    /// PKG-129: Hot Certificate Rotation without dropping server listeners or restarting
    bool rotate_certificates(const std::string& new_cert_pem, const std::string& new_key_pem) {
#ifdef PINGCORE_ENABLE_OPENSSL
        std::lock_guard<std::mutex> lock(tls_mutex_);
        if (!ssl_ctx_) {
            tls_error_ = "Cannot rotate certificates: SSL_CTX not initialized";
            return false;
        }

        BIO* cbio = BIO_new_mem_buf(new_cert_pem.data(), static_cast<int>(new_cert_pem.size()));
        X509* x509 = PEM_read_bio_X509(cbio, nullptr, nullptr, nullptr);
        BIO_free(cbio);

        if (!x509) {
            tls_error_ = "Failed to parse new X509 certificate during rotation";
            return false;
        }

        BIO* kbio = BIO_new_mem_buf(new_key_pem.data(), static_cast<int>(new_key_pem.size()));
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(kbio, nullptr, nullptr, nullptr);
        BIO_free(kbio);

        if (!pkey) {
            X509_free(x509);
            tls_error_ = "Failed to parse new private key during rotation";
            return false;
        }

        if (SSL_CTX_use_certificate(ssl_ctx_, x509) <= 0) {
            X509_free(x509);
            EVP_PKEY_free(pkey);
            tls_error_ = "Failed to install new certificate into SSL_CTX";
            return false;
        }
        X509_free(x509);

        if (SSL_CTX_use_PrivateKey(ssl_ctx_, pkey) <= 0) {
            EVP_PKEY_free(pkey);
            tls_error_ = "Failed to install new private key into SSL_CTX";
            return false;
        }
        EVP_PKEY_free(pkey);

        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            tls_error_ = "Rotated private key does not match rotated certificate";
            return false;
        }

        active_cert_pem_ = new_cert_pem;
        active_key_pem_ = new_key_pem;
        rotation_count_++;
        return true;
#else
        (void)new_cert_pem; (void)new_key_pem;
        return false;
#endif
    }

    /// Serves an existing connected client socket with TLS encryption
    void handle_tls_client_socket(int client_fd) {
#ifdef SO_NOSIGPIPE
        int nosigpipe = 1;
        ::setsockopt(client_fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
#ifdef PINGCORE_ENABLE_OPENSSL
        SSL_CTX* ctx_ref = nullptr;
        {
            std::lock_guard<std::mutex> lock(tls_mutex_);
            ctx_ref = ssl_ctx_;
        }

        if (!ctx_ref) {
            ::close(client_fd);
            return;
        }

        SSL* ssl = SSL_new(ctx_ref);
        if (!ssl) {
            ::close(client_fd);
            return;
        }

        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) > 0) {
            std::string stream_buffer;
            char buffer[8192]{};
            int bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                stream_buffer.append(buffer, static_cast<size_t>(bytes_read));
                Response res = handle_raw_request(stream_buffer);
                std::string wire_resp = res.to_http_wire_format();
                (void)SSL_write(ssl, wire_resp.data(), static_cast<int>(wire_resp.size()));
            }
            SSL_shutdown(ssl);
        }

        SSL_free(ssl);
        ::close(client_fd);
#else
        (void)client_fd;
#endif
    }

    void stop_tls() {
#ifdef PINGCORE_ENABLE_OPENSSL
        std::lock_guard<std::mutex> lock(tls_mutex_);
        if (ssl_ctx_) {
            SSL_CTX_free(ssl_ctx_);
            ssl_ctx_ = nullptr;
        }
        tls_ready_ = false;
#endif
    }

    [[nodiscard]] bool is_tls_ready() const noexcept { return tls_ready_; }
    [[nodiscard]] const std::string& tls_error() const noexcept { return tls_error_; }
    [[nodiscard]] uint32_t rotation_count() const noexcept { return rotation_count_; }
    [[nodiscard]] const std::string& active_cert_pem() const noexcept { return active_cert_pem_; }
    [[nodiscard]] const std::string& active_key_pem() const noexcept { return active_key_pem_; }

#ifdef PINGCORE_ENABLE_OPENSSL
    [[nodiscard]] SSL_CTX* ssl_ctx() const noexcept { return ssl_ctx_; }
#endif

private:
    std::mutex tls_mutex_;
#ifdef PINGCORE_ENABLE_OPENSSL
    SSL_CTX* ssl_ctx_{nullptr};
#endif
    bool tls_ready_{false};
    uint32_t rotation_count_{0};
    std::string active_cert_pem_;
    std::string active_key_pem_;
    std::string tls_error_;
};

// Client for testing and verifying real encrypted TLS 1.2/1.3 roundtrips
class HttpsClient {
public:
    static HttpsResponse request_over_socket(int fd, const std::string& raw_http_req,
                                             const std::string& verify_hostname = "",
                                             bool verify_peer = false,
                                             const std::string& ca_cert_pem = "") {
        HttpsResponse response;
#ifdef SO_NOSIGPIPE
        int nosigpipe = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
#endif
#ifdef PINGCORE_ENABLE_OPENSSL
        const SSL_METHOD* method = TLS_client_method();
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) {
            response.error = "Failed to create client SSL_CTX";
            return response;
        }

        if (verify_peer && !ca_cert_pem.empty()) {
            BIO* cbio = BIO_new_mem_buf(ca_cert_pem.data(), static_cast<int>(ca_cert_pem.size()));
            X509* ca_cert = PEM_read_bio_X509(cbio, nullptr, nullptr, nullptr);
            BIO_free(cbio);
            if (ca_cert) {
                X509_STORE* store = SSL_CTX_get_cert_store(ctx);
                X509_STORE_add_cert(store, ca_cert);
                X509_free(ca_cert);
            }
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        } else {
            // Allow self-signed test certificates when verification not explicitly enabled
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);

        if (!verify_hostname.empty()) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            SSL_set1_host(ssl, verify_hostname.c_str());
#pragma clang diagnostic pop
        }

        if (SSL_connect(ssl) <= 0) {
            unsigned long err = ERR_get_error();
            char err_buf[256]{};
            ERR_error_string_n(err, err_buf, sizeof(err_buf));
            response.error = std::string("Client SSL_connect handshake failed: ") + err_buf;
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            return response;
        }

        if (!verify_hostname.empty()) {
            X509* server_cert = SSL_get_peer_certificate(ssl);
            if (!server_cert) {
                response.error = "No server certificate presented for hostname verification";
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                return response;
            }
            X509_free(server_cert);
        }

        if (SSL_write(ssl, raw_http_req.data(), static_cast<int>(raw_http_req.size())) <= 0) {
            response.error = "Client SSL_write failed";
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            return response;
        }

        char buffer[8192]{};
        int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            response.error = "Client SSL_read failed: empty response";
        } else {
            std::string raw_resp(buffer, static_cast<size_t>(bytes));
            auto header_end = raw_resp.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                response.body = raw_resp.substr(header_end + 4);
                std::string header_section = raw_resp.substr(0, header_end);
                std::istringstream stream(header_section);
                std::string line;
                if (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    auto sp1 = line.find(' ');
                    if (sp1 != std::string::npos) {
                        auto sp2 = line.find(' ', sp1 + 1);
                        std::string code_str = (sp2 != std::string::npos) ?
                            line.substr(sp1 + 1, sp2 - sp1 - 1) :
                            line.substr(sp1 + 1);
                        try {
                            response.status_code = std::stoi(code_str);
                        } catch (...) {
                            response.status_code = 0;
                        }
                    }
                }
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    auto colon = line.find(':');
                    if (colon != std::string::npos) {
                        std::string key = line.substr(0, colon);
                        std::string val = line.substr(colon + 1);
                        auto vf = val.find_first_not_of(" \t");
                        if (vf != std::string::npos) val = val.substr(vf);
                        response.headers[key] = val;
                    }
                }
                response.success = (response.status_code >= 200 && response.status_code < 400);
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
#else
        (void)fd; (void)raw_http_req; (void)verify_hostname; (void)verify_peer; (void)ca_cert_pem;
        response.error = "OpenSSL not enabled";
#endif
        return response;
    }
};

} // namespace pingcore
