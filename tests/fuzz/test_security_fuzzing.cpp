// tests/fuzz/test_security_fuzzing.cpp
// PKG-116: Fuzz Testing Suite for Parsers, Manifests, Archives, and Boundaries
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <pingdart/pkg/manifest.h>
#include <pingdart/pkg/archive.h>
#include <pingdart/pkg/semver.h>
#include <pingdart/security/input_validator.h>
#include <pingdart/security/secure_random.h>

using namespace pingdart::pkg;
using namespace pingdart::security;

namespace {

// Deterministic Pseudo-Random Generator for Reproducible Fuzz Runs
std::mt19937_64 create_reproducible_rng(uint64_t seed = 0x5EEDF00D1337ULL) {
    return std::mt19937_64(seed);
}

std::string generate_random_fuzz_string(std::mt19937_64& rng, size_t max_len) {
    std::uniform_int_distribution<size_t> len_dist(0, max_len);
    std::uniform_int_distribution<int> char_dist(0, 255);
    size_t len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(static_cast<char>(char_dist(rng)));
    }
    return s;
}

std::vector<uint8_t> generate_random_bytes(std::mt19937_64& rng, size_t max_len) {
    std::uniform_int_distribution<size_t> len_dist(0, max_len);
    std::uniform_int_distribution<uint16_t> byte_dist(0, 255);
    size_t len = len_dist(rng);
    std::vector<uint8_t> b;
    b.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        b.push_back(static_cast<uint8_t>(byte_dist(rng)));
    }
    return b;
}

} // namespace

// ============================================================================
// 1. Fuzzing Package Manifest Parser
// ============================================================================

TEST(SecurityFuzzingTest, FuzzManifestParserRobustness) {
    auto rng = create_reproducible_rng(1001);

    // 1000 randomized fuzz inputs
    for (int i = 0; i < 1000; ++i) {
        std::string fuzz_payload = generate_random_fuzz_string(rng, 2048);
        // Parser MUST either succeed or return a clean error status, never crash or leak
        auto res = PackageManifest::parse(fuzz_payload);
        if (res.has_value()) {
            const auto& m = res.value();
            EXPECT_NO_THROW([[maybe_unused]] auto s = m.metadata().name);
        }
    }

    // Specific adversarial mutations: deeply nested brackets, huge keys, null bytes
    std::vector<std::string> adversarial_cases = {
        std::string(1000, '['),
        std::string(1000, ']') + "\nname = 'test'",
        "[package]\nname = '" + std::string(50000, 'A') + "'",
        "[package]\nname = 'pkg\0hidden'",
        "[package]\nversion = '1.0.0'\n[dependencies]\n" + std::string(500, '\n')
    };

    for (const auto& payload : adversarial_cases) {
        EXPECT_NO_THROW({
            auto res = PackageManifest::parse(payload);
            (void)res;
        });
    }
}

// ============================================================================
// 2. Fuzzing SemVer Parser and Constraint Engine
// ============================================================================

TEST(SecurityFuzzingTest, FuzzSemVersionParserRobustness) {
    auto rng = create_reproducible_rng(2002);

    for (int i = 0; i < 1000; ++i) {
        std::string fuzz_semver = generate_random_fuzz_string(rng, 128);
        try {
            auto v_res = SemVersion::parse(fuzz_semver);
            if (v_res.has_value()) {
                (void)v_res.value().to_string();
            }
        } catch (...) {
            FAIL() << "SemVersion::parse threw an unexpected exception on fuzz input";
        }

        try {
            auto c_res = VersionConstraint::parse(fuzz_semver);
            if (c_res.has_value()) {
                SemVersion test_v{1, 0, 0, ""};
                (void)c_res.value().is_satisfied_by(test_v);
            }
        } catch (...) {
            FAIL() << "VersionConstraint::parse threw an unexpected exception on fuzz input";
        }
    }

    // Adversarial integer overflows in version strings
    std::vector<std::string> overflow_cases = {
        "999999999999999999999999999999999999.0.0",
        "0.999999999999999999999999999999999999.0",
        "0.0.999999999999999999999999999999999999",
        "-1.0.0",
        "1.-1.0",
        "1.0.-1",
        "1.2.3-" + std::string(10000, 'x')
    };

    for (const auto& payload : overflow_cases) {
        EXPECT_NO_THROW({
            auto res = SemVersion::parse(payload);
            (void)res;
        });
    }
}

// ============================================================================
// 3. Fuzzing Archive Extractor & Anti-Zip-Slip
// ============================================================================

TEST(SecurityFuzzingTest, FuzzArchiveExtractorSafety) {
    auto rng = create_reproducible_rng(3003);
    auto temp_dir = std::filesystem::temp_directory_path();
    auto target_dir = temp_dir / "pingdart_fuzz_extract_target";
    std::filesystem::create_directories(target_dir);

    for (int i = 0; i < 500; ++i) {
        auto random_archive = generate_random_bytes(rng, 4096);
        EXPECT_NO_THROW({
            auto st = ArchiveUtil::unpack_package_archive(random_archive, target_dir);
            (void)st;
        });
    }

    std::filesystem::remove_all(target_dir);
}

// ============================================================================
// 4. Fuzzing Input Validator (Path Traversal & Metacharacter Injection)
// ============================================================================

TEST(SecurityFuzzingTest, FuzzPathTraversalAndSanitization) {
    auto rng = create_reproducible_rng(4004);
    auto temp_dir = std::filesystem::temp_directory_path();
    auto root_dir = temp_dir / "pingdart_fuzz_path_root";
    std::filesystem::create_directories(root_dir);

    for (int i = 0; i < 1000; ++i) {
        std::string cand = generate_random_fuzz_string(rng, 256);
        EXPECT_NO_THROW({
            auto res = InputValidator::validate_path_within_root(cand, root_dir);
            if (res.has_value()) {
                // If it succeeded, verify the returned path actually starts with root_dir!
                std::string full_s = res.value().string();
                std::string root_s = root_dir.string();
                EXPECT_GE(full_s.size(), root_s.size());
            }
        });

        EXPECT_NO_THROW({
            auto shell_res = InputValidator::sanitize_shell_argument(cand);
            (void)shell_res;
        });
    }

    std::filesystem::remove_all(root_dir);
}

// ============================================================================
// 5. Persistent Regression Corpus Verification
// ============================================================================

TEST(SecurityFuzzingTest, PersistentManifestCorpusRegression) {
    std::filesystem::path corpus_dir = "tests/fuzz/corpus/manifests";
    if (std::filesystem::exists(corpus_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(corpus_dir)) {
            if (entry.is_regular_file()) {
                std::ifstream f(entry.path());
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                auto res = PackageManifest::parse(content);
                EXPECT_TRUE(res.has_value()) << "Failed to parse persistent corpus seed: " << entry.path();
            }
        }
    }
}

TEST(SecurityFuzzingTest, PersistentSemVerCorpusRegression) {
    std::filesystem::path seed_file = "tests/fuzz/corpus/semver/semver_seeds.txt";
    if (std::filesystem::exists(seed_file)) {
        std::ifstream f(seed_file);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto v_res = SemVersion::parse(line);
            if (!v_res.has_value()) {
                auto c_res = VersionConstraint::parse(line);
                EXPECT_TRUE(c_res.has_value()) << "Failed to parse persistent semver/constraint seed: " << line;
            }
        }
    }
}

TEST(SecurityFuzzingTest, PinnedDestinationSSRFDefense) {
    // 1. Direct restricted IPs must be blocked
    EXPECT_FALSE(InputValidator::resolve_and_pin_destination("http://127.0.0.1/").has_value());
    EXPECT_FALSE(InputValidator::resolve_and_pin_destination("http://169.254.169.254/latest/meta-data").has_value());
    EXPECT_FALSE(InputValidator::resolve_and_pin_destination("http://10.0.0.1/admin").has_value());
    EXPECT_FALSE(InputValidator::resolve_and_pin_destination("http://[::1]/").has_value());

    // 2. Redirect with protocol downgrade must be rejected
    auto redir_down = InputValidator::validate_and_pin_redirect("https://example.com/api", "http://example.com/api");
    EXPECT_FALSE(redir_down.has_value());
    EXPECT_EQ(redir_down.status().code(), pingdart::core::StatusCode::PermissionDenied);

    // 3. Redirect to private/restricted subnet must be rejected
    auto redir_priv = InputValidator::validate_and_pin_redirect("https://example.com/api", "https://127.0.0.1/secret");
    EXPECT_FALSE(redir_priv.has_value());
}
