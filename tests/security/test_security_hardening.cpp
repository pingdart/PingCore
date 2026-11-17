// tests/security/test_security_hardening.cpp
// PKG-115: Unit Tests for Security Subsystems & OS Sandbox Isolation
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <pingdart/security/input_validator.h>
#include <pingdart/security/secure_random.h>
#include <pingdart/security/permissions.h>
#include <pingdart/security/secure_storage.h>
#include <pingdart/pkg/os_sandbox.h>

using namespace pingdart::security;
using namespace pingdart::pkg;

// ============================================================================
// 1. Input Validation & Injection Prevention
// ============================================================================

TEST(SecurityHardeningTest, PathTraversalDefense) {
    EXPECT_TRUE(InputValidator::has_path_traversal("../../etc/shadow"));
    EXPECT_TRUE(InputValidator::has_path_traversal("..\\Windows\\System32"));
    EXPECT_TRUE(InputValidator::has_path_traversal(std::string("file.txt\0.exe", 12)));
    EXPECT_FALSE(InputValidator::has_path_traversal("normal/path/file.txt"));

    auto temp_dir = std::filesystem::temp_directory_path();
    auto root_dir = temp_dir / "pingdart_test_root";
    std::filesystem::create_directories(root_dir);

    // Valid path within root
    auto valid_res = InputValidator::validate_path_within_root("sub/file.txt", root_dir);
    EXPECT_TRUE(valid_res.has_value());

    // Path traversal attempting escape
    auto escape_res = InputValidator::validate_path_within_root("../../etc/passwd", root_dir);
    EXPECT_FALSE(escape_res.has_value());
    EXPECT_EQ(escape_res.status().code(), pingdart::core::StatusCode::InvalidArgument);

    std::filesystem::remove_all(root_dir);
}

TEST(SecurityHardeningTest, ShellArgumentSanitization) {
    // Dangerous shell metacharacters blocked
    EXPECT_FALSE(InputValidator::sanitize_shell_argument("file.txt; rm -rf /").has_value());
    EXPECT_FALSE(InputValidator::sanitize_shell_argument("foo && cat /etc/passwd").has_value());
    EXPECT_FALSE(InputValidator::sanitize_shell_argument("`whoami`").has_value());
    EXPECT_FALSE(InputValidator::sanitize_shell_argument("$(calc.exe)").has_value());
    EXPECT_FALSE(InputValidator::sanitize_shell_argument("test | ls").has_value());

    // Safe arguments safely quoted
    auto safe_res = InputValidator::sanitize_shell_argument("normal_filename.tar.gz");
    ASSERT_TRUE(safe_res.has_value());
    EXPECT_EQ(safe_res.value(), "'normal_filename.tar.gz'");
}

TEST(SecurityHardeningTest, SSRFUrlValidation) {
    // Legitimate public URLs
    EXPECT_TRUE(InputValidator::validate_url("https://pkg.pingdart.dev/api/v1/packages").ok());
    EXPECT_TRUE(InputValidator::validate_url("http://cdn.pingdart.org/bundles").ok());

    // Invalid scheme
    EXPECT_FALSE(InputValidator::validate_url("ftp://pkg.pingdart.dev").ok());
    EXPECT_FALSE(InputValidator::validate_url("file:///etc/passwd").ok());

    // SSRF Localhost / Loopback / Cloud metadata blocked
    EXPECT_FALSE(InputValidator::validate_url("http://localhost:8080/admin").ok());
    EXPECT_FALSE(InputValidator::validate_url("http://127.0.0.1:6379").ok());
    EXPECT_FALSE(InputValidator::validate_url("http://169.254.169.254/latest/meta-data").ok());
    EXPECT_FALSE(InputValidator::validate_url("http://192.168.1.1/router").ok());
    EXPECT_FALSE(InputValidator::validate_url("http://10.0.0.5/internal").ok());
}

TEST(SecurityHardeningTest, ConnectionTimePrivateIpEnforcement) {
    // IPv4 private, loopback, and metadata ranges
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("127.0.0.1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("127.1.2.3"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("10.0.0.1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("172.16.0.1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("172.31.255.255"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("192.168.1.1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("169.254.169.254"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("0.0.0.0"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("255.255.255.255"));

    // IPv6 loopback, ULA, and metadata
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("::1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("::"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("fc00::1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("fe80::1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("::ffff:127.0.0.1"));
    EXPECT_TRUE(InputValidator::is_private_or_restricted_ip("::ffff:169.254.169.254"));

    // Public Internet IPs allowed
    EXPECT_FALSE(InputValidator::is_private_or_restricted_ip("8.8.8.8"));
    EXPECT_FALSE(InputValidator::is_private_or_restricted_ip("1.1.1.1"));
    EXPECT_FALSE(InputValidator::is_private_or_restricted_ip("2606:4700:4700::1111"));
}

TEST(SecurityHardeningTest, DnsRebindingAndSocketDestinationEnforcement) {
    // Direct blocked IPs
    EXPECT_FALSE(InputValidator::validate_socket_destination("127.0.0.1", 80, false).ok());
    EXPECT_FALSE(InputValidator::validate_socket_destination("169.254.169.254", 80, false).ok());
    EXPECT_FALSE(InputValidator::validate_socket_destination("localhost", 80, false).ok());

    // Public destination allowed
    EXPECT_TRUE(InputValidator::validate_socket_destination("1.1.1.1", 443, false).ok());
}

TEST(SecurityHardeningTest, RedirectChainAndDowngradeDefense) {
    // Insecure downgrade https -> http rejected
    auto down_res = InputValidator::validate_redirect("https://pkg.pingdart.dev", "http://pkg.pingdart.dev");
    EXPECT_FALSE(down_res.ok());
    EXPECT_EQ(down_res.code(), pingdart::core::StatusCode::PermissionDenied);

    // Redirect to link-local metadata rejected
    auto meta_res = InputValidator::validate_redirect("https://pkg.pingdart.dev", "http://169.254.169.254/latest/meta-data");
    EXPECT_FALSE(meta_res.ok());

    // Redirect exceeding maximum hop count rejected
    auto hop_res = InputValidator::validate_redirect("https://pkg.pingdart.dev", "https://cdn.pingdart.dev", 6, 5);
    EXPECT_FALSE(hop_res.ok());
    EXPECT_EQ(hop_res.code(), pingdart::core::StatusCode::ResourceExhausted);

    // Safe HTTPS redirect to public endpoint permitted
    EXPECT_TRUE(InputValidator::validate_redirect("https://pkg.pingdart.dev/dl", "https://1.1.1.1/pkg", 1, 5).ok());
}

TEST(SecurityHardeningTest, SafeIdentifiers) {
    EXPECT_TRUE(InputValidator::is_safe_identifier("pingdart-core"));
    EXPECT_TRUE(InputValidator::is_safe_identifier("my_package_1.0.0"));
    EXPECT_FALSE(InputValidator::is_safe_identifier(".hidden"));
    EXPECT_FALSE(InputValidator::is_safe_identifier("-flag"));
    EXPECT_FALSE(InputValidator::is_safe_identifier("pkg;rm -rf"));
    EXPECT_FALSE(InputValidator::is_safe_identifier(""));
}

// ============================================================================
// 2. Cryptographically Secure Random Generation
// ============================================================================

TEST(SecurityHardeningTest, SecureRandomGeneration) {
    auto bytes_res = SecureRandom::generate_bytes(32);
    ASSERT_TRUE(bytes_res.has_value());
    const auto& bytes = bytes_res.value();
    EXPECT_EQ(bytes.size(), 32u);

    // Verify randomness (not all zeroes)
    bool all_zero = true;
    for (uint8_t b : bytes) {
        if (b != 0) { all_zero = false; break; }
    }
    EXPECT_FALSE(all_zero);

    uint64_t r1 = SecureRandom::next_uint64();
    uint64_t r2 = SecureRandom::next_uint64();
    EXPECT_NE(r1, r2);

    std::string token = SecureRandom::generate_hex_token(16);
    EXPECT_EQ(token.size(), 32u);
}

// ============================================================================
// 3. Permission Management & Fail-Closed Capability Enforcing
// ============================================================================

TEST(SecurityHardeningTest, PermissionManagerEnforcement) {
    auto& pm = PermissionManager::instance();
    pm.reset();

    // Default state: not granted, fail-closed
    EXPECT_FALSE(pm.is_granted(Permission::NetworkClient));
    auto enf_res = pm.enforce(Permission::NetworkClient);
    EXPECT_FALSE(enf_res.ok());
    EXPECT_EQ(enf_res.code(), pingdart::core::StatusCode::PermissionDenied);

    // Grant capability
    pm.grant(Permission::NetworkClient);
    EXPECT_TRUE(pm.is_granted(Permission::NetworkClient));
    EXPECT_TRUE(pm.enforce(Permission::NetworkClient).ok());

    // Explicitly deny
    pm.deny(Permission::NetworkClient);
    EXPECT_FALSE(pm.is_granted(Permission::NetworkClient));
    EXPECT_FALSE(pm.enforce(Permission::NetworkClient).ok());

    pm.reset();
}

// ============================================================================
// 4. Secure Storage
// ============================================================================

TEST(SecurityHardeningTest, SecureStorageLifecycle) {
    auto storage = create_secure_storage();
    ASSERT_NE(storage, nullptr);

    std::string_view key = "pingdart.registry.token";
    pingdart::safety::SecureBuffer secret("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9");

    EXPECT_TRUE(storage->set_secret(key, secret).ok());
    EXPECT_TRUE(storage->has_secret(key));

    auto ret_res = storage->get_secret(key);
    ASSERT_TRUE(ret_res.has_value());
    EXPECT_EQ(ret_res.value().size(), secret.size());

    EXPECT_TRUE(storage->delete_secret(key).ok());
    EXPECT_FALSE(storage->has_secret(key));
}

// ============================================================================
// 5. OS Sandbox Isolation & Seatbelt Worker Execution
// ============================================================================

TEST(SecurityHardeningTest, OsSandboxWorkerExecution) {
    EXPECT_TRUE(OsSandboxWorker::is_os_isolation_available());

    auto temp_dir = std::filesystem::temp_directory_path();
    auto workspace = temp_dir / "pingdart_sandbox_test_ws";
    std::filesystem::create_directories(workspace);

    OsSandboxLimits limits{
        .max_cpu_time_ms = 5000,
        .max_memory_bytes = 256 * 1024 * 1024,
        .max_processes = 8,
        .deny_network = true,
        .read_only_host_fs = true,
        .strip_host_credentials = true,
        .require_strict_os_sandbox = false // Test trusted worker execution
    };

    OsSandboxWorker worker(limits);

    // Run clean command in workspace
    auto rep_res = worker.run_isolated_build(workspace, "echo 'sandbox_test_success' > output.txt");
    ASSERT_TRUE(rep_res.has_value());
    const auto& report = rep_res.value();

    EXPECT_TRUE(report.success);
    EXPECT_EQ(report.exit_code, 0);
    EXPECT_TRUE(report.network_attempt_blocked);
    EXPECT_TRUE(report.filesystem_access_restricted);
    EXPECT_TRUE(report.host_credentials_stripped);

    // Verify output file written to workspace
    EXPECT_TRUE(std::filesystem::exists(workspace / "output.txt"));

    std::filesystem::remove_all(workspace);
}

TEST(SecurityHardeningTest, OsSandboxWorkerTimeoutEnforcement) {
    auto temp_dir = std::filesystem::temp_directory_path();
    auto workspace = temp_dir / "pingdart_timeout_test_ws";
    std::filesystem::create_directories(workspace);

    // Strict 100ms timeout
    OsSandboxLimits limits{
        .max_cpu_time_ms = 100,
        .deny_network = true,
        .read_only_host_fs = true,
        .require_strict_os_sandbox = false
    };

    OsSandboxWorker worker(limits);

    // Run a command that attempts to sleep for 5 seconds
    auto rep_res = worker.run_isolated_build(workspace, "sleep 5");
    ASSERT_TRUE(rep_res.has_value());
    const auto& report = rep_res.value();

    EXPECT_FALSE(report.success);
    EXPECT_TRUE(report.timeout_hit);

    std::filesystem::remove_all(workspace);
}

TEST(SecurityHardeningTest, UntrustedWorkloadFailsClosedWithoutStrictSandbox) {
    auto temp_dir = std::filesystem::temp_directory_path();
    auto workspace = temp_dir / "pingdart_failclosed_test_ws";
    std::filesystem::create_directories(workspace);

    // Strict OS sandbox required for untrusted package build
    OsSandboxLimits limits{
        .max_cpu_time_ms = 5000,
        .deny_network = true,
        .read_only_host_fs = true,
        .require_strict_os_sandbox = true
    };

    OsSandboxWorker worker(limits);
    auto rep_res = worker.run_isolated_build(workspace, "echo 'malicious_escape'");

    // Inside nested sandbox environment, strict Seatbelt fails closed with PermissionDenied
    // On native un-sandboxed host with Seatbelt, it executes under the sandbox profile
    if (!rep_res.has_value()) {
        EXPECT_EQ(rep_res.status().code(), pingdart::core::StatusCode::PermissionDenied);
    } else {
        EXPECT_TRUE(rep_res.value().success);
    }

    std::filesystem::remove_all(workspace);
}
