# PingDart Framework Security Threat Model
**PKG-116: Comprehensive Threat Modeling, Risk Mitigation & Security Assurance**
**Revision**: 1.1 (November 2026)

---

## 1. Executive Summary
PingDart is a high-performance cross-platform application framework written in C++20 with a Rust-based compiler subsystem, supporting macOS, iOS, Android, Linux, Windows, and Web. This document details the threat landscape, trust boundaries, threat actors, and defense-in-depth mitigations implemented across the runtime, packaging ecosystem, platform abstraction layers, and native rendering pipelines.

---

## 2. Trust Boundaries & Architecture
PingDart divides application execution into discrete privilege domains:

```
[ Untrusted Internet / Package Registry ]
                  │
                  ▼ (TLS 1.3, SHA-256 Digest Verification, Ed25519 Publisher Signatures)
┌────────────────────────────────────────────────────────┐
│               PingDart Package Manager                 │
│         (Dependency Cycle Detection, SemVer)           │
│    (SSRF & DNS Rebinding Socket-Level Validation)      │
└────────────────────────────────────────────────────────┘
                  │
                  ▼ (OS Sandbox: Fail-Closed Seatbelt / AppContainer / Namespaces)
┌────────────────────────────────────────────────────────┐
│             Isolated Package Build Worker              │
│       (Read-only Root FS, Network Denied, rlimits)     │
└────────────────────────────────────────────────────────┘
                  │
                  ▼ (Compiler FFI Boundary & Static Verification)
┌────────────────────────────────────────────────────────┐
│            PingDart Core Engine & RHI / PAL            │
│  (SafeSpan, NonNull, Checked Math, Strict RefPtr RAII) │
└────────────────────────────────────────────────────────┘
```

---

## 3. Threat Classification & Mitigations (STRIDE Matrix)

### 3.1 Spoofing
- **Threat**: Malicious actor masquerades as a trusted publisher, hijacks scoped namespace packages (`@official/...`), or triggers DNS rebinding to forge upstream responses.
- **Mitigation**:
  - Ed25519 cryptographic package signatures.
  - Organization verification with role-based publisher authentication.
  - Strict SHA-256 content addressing in lockfiles (`pingdart.lock`).
  - Socket-level destination IP verification before establishing connections, rejecting private IP ranges regardless of DNS responses.

### 3.2 Tampering
- **Threat**: In-flight modification of package archives, font files, display lists, or archive traversal attacks.
- **Mitigation**:
  - Constant-time SHA-256 digest checks before decompression.
  - Archive extraction validates path safety (anti-Zip Slip path traversal prevention in `InputValidator::validate_path_within_root`).
  - Immutable display-list command sequences.
  - Automated continuous fuzzing of archive decoders and manifest parsers.

### 3.3 Repudiation
- **Threat**: Publisher claims they did not publish a compromised version.
- **Mitigation**:
  - Cryptographically signed release receipts and immutable audit logs with timestamped publisher tokens.

### 3.4 Information Disclosure & SSRF
- **Threat**: Memory dumping of sensitive tokens, private keys, or Server-Side Request Forgery (SSRF) targeting cloud metadata services (e.g., `169.254.169.254`, IPv6 `fe80::`, or `::ffff:169.254.169.254`).
- **Mitigation**:
  - `SecureBuffer` wipes secret contents immediately upon destruction (`memset_s` / `explicit_bzero`).
  - Memory page locking via `mlock` / `VirtualLock` prevents swapping credentials to disk.
  - Ambient environment variables (`AWS_*`, `TOKEN`, `KEY`) are stripped before worker process execution.
  - Socket destination check blocks RFC1918, loopback, link-local, cloud metadata, and IPv4-mapped IPv6 subnets.
  - Redirect policies strictly enforce TLS, disallowing scheme downgrades (`https` -> `http`) and re-validating final resolved IPs.

### 3.5 Denial of Service
- **Threat**: Recursive dependency loops, zip bombs, memory exhaustion, fork bombs, GPU shader hangs, or concurrency race conditions.
- **Mitigation**:
  - Cycle-detecting topological DFS resolver (`resolver.cpp`).
  - OS-level resource limits (`RLIMIT_CPU`, `RLIMIT_NPROC`, `RLIMIT_NOFILE`, memory quotas).
  - Metal/Vulkan fence timeouts and swapchain frame throttles.
  - ThreadSanitizer (TSan) verification ensuring 100% race-free concurrency across event loops and rendering threads.

### 3.6 Elevation of Privilege
- **Threat**: Untrusted build script or plugin executing arbitrary code with full host privileges when sandboxing is degraded or unavailable.
- **Mitigation**:
  - **macOS**: Apple Seatbelt policy (`(deny default)`, strictly designated workspace subpath, network denied).
  - **Windows**: AppContainer profile isolation, Restricted Tokens, and Job Objects.
  - **Linux**: Unprivileged user namespaces (`CLONE_NEWNS`, `CLONE_NEWNET`) and Seccomp syscall filtering.
  - **Fail-Closed Policy**: Untrusted package jobs MUST NOT silently fall back to unprivileged POSIX execution if OS-level isolation (e.g. Seatbelt) cannot be applied. Missing sandbox infrastructure results in immediate job rejection (`StatusCode::PermissionDenied`).

---

## 4. Security Verification Gates
All changes must satisfy:
1. `ASan` + `UBSan`: 100% passing test matrix with zero memory safety violations or undefined behavior.
2. `TSan`: 100% passing test matrix with zero detected race conditions or thread synchronization defects.
3. Negative Security Tests: 100% pass on negative sandbox, path traversal, Zip Slip, SSRF, DNS rebinding, and permission failure suites.
4. Fuzzing Gate: Automated randomized fuzzing of manifests, version constraints, and archive unpacking with zero crashes.
5. Compiler Hardening: Compiled with `-fstack-protector-strong`, `_FORTIFY_SOURCE=2`, and PIE/ASLR.
