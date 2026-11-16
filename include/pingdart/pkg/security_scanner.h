// include/pingdart/pkg/security_scanner.h
// PKG-109: Package Scanning, Advisories and Publisher Security
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <cstdint>
#include <pingdart/core/result.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/pkg/manifest.h>

#include <mutex>

namespace pingdart::pkg {

enum class ScanSeverity {
    Info,
    Low,
    Medium,
    High,
    Critical
};

[[nodiscard]] std::string scan_severity_to_string(ScanSeverity sev);

struct ScanFinding {
    std::string rule_id;
    ScanSeverity severity{ScanSeverity::Low};
    std::string file_path;
    std::string description;
};

struct ScanReport {
    bool is_clean{true};
    size_t total_files_scanned{0};
    uint64_t total_bytes_scanned{0};
    int risk_score{0}; // 0 = safe, 100 = critical threat
    std::vector<ScanFinding> findings;

    [[nodiscard]] bool has_critical_or_high() const noexcept;
    [[nodiscard]] std::string format_summary() const;
};

// ============================================================================
// PKG-109: Package Static Security & Archive Scanner
// ============================================================================

class SecurityScanner {
public:
    SecurityScanner() = default;

    // Scan raw package archive (.pdar) before extraction or publishing
    [[nodiscard]] ScanReport scan_archive_payload(
        const std::vector<uint8_t>& archive_data,
        const PackageManifest* manifest = nullptr
    );

    // Scan an uncompressed source tree directory
    [[nodiscard]] ScanReport scan_directory(
        const std::filesystem::path& dir_path,
        const PackageManifest* manifest = nullptr
    );

    // Scan a single CMakeLists.txt or .cmake script for untrusted execution commands
    [[nodiscard]] std::vector<ScanFinding> scan_cmake_script(
        std::string_view script_content,
        std::string_view file_path = "CMakeLists.txt"
    );

    // Inspect byte buffer for known compiled binary magic signatures
    [[nodiscard]] static bool is_compiled_binary(const uint8_t* data, size_t length);

    // Check if filename has an executable/binary file extension
    [[nodiscard]] static bool is_banned_executable_extension(std::string_view filename);

    // Check if path indicates sensitive or dangerous system files
    [[nodiscard]] static bool is_sensitive_or_dangerous_path(std::string_view rel_path);

private:
    static constexpr uint64_t MAX_UNCOMPRESSED_ARCHIVE_SIZE = 50 * 1024 * 1024; // 50MB
    static constexpr uint64_t MAX_SINGLE_FILE_SIZE = 20 * 1024 * 1024;          // 20MB
};

// ============================================================================
// PKG-109: Security Advisory Database Engine
// ============================================================================

struct ExtendedSecurityAdvisory {
    std::string advisory_id;      // e.g. PINGDART-2026-0001, CVE-2026-10294
    std::string package_name;
    VersionConstraint affected_constraint;
    std::string patched_version;
    ScanSeverity severity{ScanSeverity::High};
    std::string title;
    std::string description;
    std::string cwe_id;           // e.g. CWE-89, CWE-22, CWE-78
    std::string published_at;     // ISO timestamp
};

class AdvisoryDatabase {
public:
    AdvisoryDatabase();

    void add_advisory(ExtendedSecurityAdvisory advisory);
    [[nodiscard]] std::vector<ExtendedSecurityAdvisory> query_by_package(std::string_view package_name) const;
    [[nodiscard]] std::vector<ExtendedSecurityAdvisory> check_version(std::string_view package_name, const SemVersion& version) const;
    [[nodiscard]] bool has_critical_or_high(std::string_view package_name, const SemVersion& version) const;
    [[nodiscard]] const std::vector<ExtendedSecurityAdvisory>& all_advisories() const noexcept;

private:
    std::vector<ExtendedSecurityAdvisory> advisories_;
    void seed_default_advisories();
};

// ============================================================================
// PKG-109: Scoped Publishing Tokens & Publisher Ownership Security
// ============================================================================

struct PublishingToken {
    std::string token_id;
    std::string token_hash; // SHA-256 hash of secret (stored nonreversibly)
    std::string publisher_username;
    std::vector<std::string> scopes; // e.g. "pkg:publish", "pkg:read", "admin"
    std::string created_at;
    uint64_t expires_at_epoch{0};
    bool is_revoked{false};

    [[nodiscard]] bool has_scope(std::string_view scope) const;
    [[nodiscard]] bool is_valid(uint64_t current_epoch = 0) const;
};

struct TokenCreationResult {
    PublishingToken token;
    std::string plaintext_secret; // Returned ONCE to client
};

[[nodiscard]] std::string generate_csprng_token();

struct SecurityAuditEntry {
    std::string timestamp;
    std::string actor;           // publisher username or system
    std::string action;          // "publish", "token_create", "collaborator_add", "scan_rejected"
    std::string package_name;
    std::string version;
    std::string status;          // "success", "failed", "rejected", "denied"
    std::string details;
    std::string prev_hash;       // Cryptographic tamper-detection hash chain
    std::string entry_hash;

    [[nodiscard]] std::string compute_hash() const;
};

class PublisherSecurity {
public:
    PublisherSecurity();

    // Accounts
    core::Status register_publisher(std::string_view username, std::string_view email, bool verified = false);
    [[nodiscard]] bool publisher_exists(std::string_view username) const;
    [[nodiscard]] bool is_verified_publisher(std::string_view username) const;

    // Tokens (Finding C: Nonreversible hashing, expiry, revocation, concurrency protection)
    [[nodiscard]] core::Result<TokenCreationResult> create_token(
        std::string_view username,
        const std::vector<std::string>& scopes,
        uint32_t valid_days = 90
    );
    [[nodiscard]] core::Result<std::string> authenticate_token(
        std::string_view raw_token_secret,
        std::string_view required_scope = "pkg:publish"
    );
    core::Status revoke_token(std::string_view token_secret_or_id);

    // Ownership & Collaboration (Finding C: Transactional mutex)
    core::Status claim_or_verify_ownership(std::string_view username, std::string_view package_name);
    [[nodiscard]] bool can_publish(std::string_view username, std::string_view package_name) const;
    [[nodiscard]] std::vector<std::string> get_package_owners(std::string_view package_name) const;
    core::Status add_collaborator(std::string_view requester, std::string_view package_name, std::string_view new_owner);

    // Audit Logging
    void record_audit_event(SecurityAuditEntry entry);
    [[nodiscard]] std::vector<SecurityAuditEntry> get_audit_log(std::string_view package_name = "") const;
    [[nodiscard]] bool verify_audit_log_integrity() const;

private:
    struct PublisherRecord {
        std::string username;
        std::string email;
        bool is_verified{false};
    };

    mutable std::mutex ownership_mutex_;
    mutable std::mutex token_mutex_;
    std::map<std::string, PublisherRecord> publishers_;
    std::map<std::string, PublishingToken> tokens_by_hash_; // SHA-256 hash -> token
    std::map<std::string, std::set<std::string>> package_owners_; // package_name -> owners
    std::vector<SecurityAuditEntry> audit_log_;
    void seed_default_publishers();
};

} // namespace pingdart::pkg
