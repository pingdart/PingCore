# PingDart Framework
## DOC-06: Plugin SDK & Security Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20 · C-ABI 1.0  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01](doc-01-core-runtime-architecture.md) · [DOC-03](doc-03-platform-adapter-sdk.md) · [DOC-04](doc-04-compiler-build-system.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Plugin SDK & Security Architecture** provides a safe, extensible framework for third-party native capabilities without compromising host stability, memory safety, or user security.

A foundational principle of this architecture is that **a separate process is not automatically a sandbox**. PingDart establishes a multi-tiered security perimeter:
1. **The Plugin Manager:** Discovers packages, parses manifests, negotiates versions, and verifies digital signatures.
2. **The Capability Broker:** Enforces least-privilege access, mediates all requests to platform resources (files, network, camera), and logs security audits.
3. **Dual Execution Models:**
   - **Trusted In-Process Plugins:** High-performance, engine-verified modules running via a stable, versioned C-ABI.
   - **Sandboxed Out-of-Process Plugins:** Untrusted third-party code isolated inside an OS sandbox (Windows AppContainer / Linux namespaces + seccomp) communicating strictly via authenticated IPC.

```text
+--------------------------------------------------------------------------+
|                        PingDart Application                              |
|                    Requests Plugin Feature / API                         |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                          Plugin Manager                                  |
|         Discovery · Manifest Validation · Version Negotiation            |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                         Capability Broker                                |
|        Permission Enforcement · Resource Mediation · Audit Log           |
+--------------------------------------------------------------------------+
                   |                                     |
    [In-Process Mode]                     [Sandboxed Process Mode]
                   |                                     |
                   v                                     v
+-----------------------------------+  +-----------------------------------+
|          Trusted Plugins          |  |         Untrusted Plugins         |
|  - In-Process Shared Library      |  |  - Isolated Child Process         |
|  - Versioned C-ABI                |  |  - Windows Job / Linux Seccomp    |
|  - Zero-Copy Direct Execution     |  |  - Unix Domain Socket / Pipe IPC  |
|  - For Official Engine Modules    |  |  - Shared-Memory Ring Buffers     |
+-----------------------------------+  +-----------------------------------+
```

---

# 2. Plugin Manifest Schema (`plugin.yaml`)

Every PingDart plugin must include a top-level `plugin.yaml` manifest. The manifest defines identity, required capabilities, resource constraints, and runtime isolation settings:

```yaml
schema_version: 1

plugin:
  id: com.pingdart.camera
  name: Native Camera Capture
  version: 1.0.0
  abi_version: 1
  description: "High-performance camera capture and frame streaming plugin"
  author: "PingDart Core Team <dev@pingdart.dev>"
  license: "Apache-2.0"

runtime:
  isolation: process     # Options: "process" (sandboxed child) or "in_process" (trusted only)
  entrypoint: camera_plugin_runner
  supported_platforms:
    - windows
    - linux

permissions:
  - camera.capture
  - storage.cache
  # Unauthorized permissions (e.g. network.raw_sockets) will be rejected at load time

resources:
  memory_limit_mb: 256
  max_concurrent_requests: 32
  cpu_quota_percent: 50

dependencies:
  - id: com.pingdart.media_core
    version: "^1.0.0"
```

---

# 3. Stable C-ABI Contract (`pd_plugin_abi.h`)

To insulate plugins from C++ compiler ABI incompatibilities (MSVC vs. Clang, C++ standard library layout shifts), the host-plugin interface uses a stable, versioned C interface.

```c
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINGDART_PLUGIN_ABI_VERSION 1

typedef enum pd_plugin_status_t {
    PD_PLUGIN_OK = 0,
    PD_PLUGIN_ERR_INVALID_ARGUMENT = 1,
    PD_PLUGIN_ERR_PERMISSION_DENIED = 2,
    PD_PLUGIN_ERR_UNSUPPORTED = 3,
    PD_PLUGIN_ERR_TIMEOUT = 4,
    PD_PLUGIN_ERR_INTERNAL = 5
} pd_plugin_status_t;

// Opaque handles
typedef struct pd_host_broker_t pd_host_broker_t;
typedef struct pd_plugin_context_t pd_plugin_context_t;

// Host Capability Broker Callback (Plugin calls host)
typedef pd_plugin_status_t (*pd_host_request_fn)(
    pd_host_broker_t* broker,
    const char* permission_name,
    const char* method_name,
    const uint8_t* request_payload,
    size_t request_len,
    uint8_t** out_response_payload,
    size_t* out_response_len
);

// Plugin Exported Functions
uint32_t pd_plugin_get_abi_version(void);

pd_plugin_status_t pd_plugin_initialize(
    pd_host_broker_t* broker,
    pd_host_request_fn request_fn,
    pd_plugin_context_t** out_context
);

pd_plugin_status_t pd_plugin_invoke(
    pd_plugin_context_t* context,
    const char* method_name,
    const uint8_t* payload,
    size_t payload_len,
    uint8_t** out_response,
    size_t* out_response_len
);

void pd_plugin_destroy(pd_plugin_context_t* context);
void pd_plugin_free_buffer(uint8_t* buffer);

#ifdef __cplusplus
}
#endif
```

---

# 4. Plugin Lifecycle State Machine

```text
               [Unloaded / On-Disk]
                        |
                  pd_plugin_load()
                        v
                  [Loaded / Idle]
                        |
               on_initialize(broker)
                        v
                 [Active / Ready] <------+
                        |                | on_resume()
                  on_suspend()           |
                        v                |
                  [Suspended] -----------+
                        |
                  on_destroy()
                        v
              [Defunct / Terminated]
```

1. **`on_load`:** Host verifies digital signature and manifest schema.
2. **`on_initialize(broker)`:** Context instantiated; host registers permitted capabilities.
3. **`on_activate`:** Plugin enters active execution, begins listening for incoming requests.
4. **`on_suspend`:** Host notifies plugin to release temporary memory/devices (e.g., when app minimizes).
5. **`on_resume`:** Plugin restores execution state.
6. **`on_destroy`:** Plugin reclaims all local allocations and handles cleanly before unloading.

---

# 5. Capability Broker & Permission Architecture

The **Capability Broker** mediates every request to sensitive resources:

```text
Plugin Context
      |
      | invoke("camera.capture", frame_request)
      v
Capability Broker
      |
      |-- 1. Check Manifest: Is "camera.capture" declared?
      |-- 2. Check User Policy: Has user granted camera access?
      |-- 3. Rate Limiter: Has plugin exceeded max_concurrent_requests?
      |
      +---> [Approved]  --> Executes native hardware capture
      +---> [Denied]    --> Returns PD_PLUGIN_ERR_PERMISSION_DENIED
```

### 5.1 Security Audit Logging
Every privileged platform access brokered to a plugin generates a structured audit log:
```json
{
  "timestamp_utc": "2026-11-16T11:25:00Z",
  "plugin_id": "com.pingdart.camera",
  "capability": "camera.capture",
  "method": "start_capture",
  "result": "APPROVED",
  "latency_us": 420
}
```

---

# 6. Operating System Sandboxing Architecture

For all untrusted plugins (`isolation: process`), PingDart enforces strict OS-level containerization:

### 6.1 Windows Sandboxing
1. **Restricted Tokens (`CreateRestrictedToken`):**
   - Strips administrative and elevated privileges.
   - Disables all non-essential security SIDs.
   - Enforces `SECURITY_MANDATORY_LOW_RID` (Low Integrity Level), preventing writes to user documents, system directories, and the registry.
2. **Job Objects (`CreateJobObjectW`):**
   - Enforces memory limit via `JOBOBJECT_EXTENDED_LIMIT_INFORMATION::ProcessMemoryLimit`.
   - Sets `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` ensuring child processes terminate automatically when the host closes the job handle.
   - Child process startup executes `SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS)` to suppress interactive Windows crash dialogs.
   - The host monitors the child process handle with `RegisterWaitForSingleObject` to capture exit codes immediately.
3. **AppContainer Isolation Profile:**
   - Evaluated for store distribution to completely isolate filesystem and network access except through brokered capability capabilities.

### 6.2 Linux Sandboxing & Fail-Closed Enforcement
1. **Linux Namespaces (`clone` / `unshare`):**
   - `CLONE_NEWPID`: Process cannot inspect or signal any other processes on the host.
   - `CLONE_NEWNET`: Disables all raw networking unless explicit network capability is granted.
   - `CLONE_NEWNS`: Mount namespace with an isolated, read-only root (`chroot` / `pivot_root`).
2. **Seccomp-BPF Syscall Filtering:**
   - Installs a restrictive BPF filter allowing only essential compute syscalls (`read`, `write`, `futex`, `exit_group`).
   - Explicitly blocks and kills on `execve`, `fork`, `clone`, `ptrace`, and raw socket creation.
3. **Cgroups v2 Resource Quotas:**
   - Enforces hard memory boundaries (`memory.max`) and CPU quotas (`cpu.max`).
4. **Mandatory Fail-Closed Policy:**
   - *Security Invariant:* Operating system sandboxing capabilities vary across Linux distributions, custom kernels, and virtualized Docker/CI containers.
   - If the host detects that unprivileged user namespaces, seccomp-bpf, or cgroups v2 are missing or restricted, sandbox initialization **strictly fails closed**.
   - PingDart **never** silently degrades to running untrusted native code unconfined; it immediately aborts loading with `PD_PLUGIN_ERR_UNSUPPORTED`.

---

# 7. Inter-Process Communication (IPC) & Memory Safety

For out-of-process plugins, PingDart utilizes an authenticated, high-throughput IPC transport:

```text
+-----------------------+                         +-----------------------+
|  Host Process (PAL)   |                         |   Sandboxed Plugin    |
+-----------------------+                         +-----------------------+
           |                                                  |
           |---- Peer Authentication (SO_PEERCRED / Pipe) --->|
           |<--- Session Nonce Handshake ---------------------|
           |                                                  |
           |---- Framed Protocol Buffers / Flatbuffers ------>|
           |<--- Authenticated Method Responses --------------|
           |                                                  |
           |====== Shared Memory Ring Buffer (Zero-Copy) =====|
           |    (Atomic Acquire/Release · Bounds Checked)     |
```

### 7.1 Peer Identity Authentication & Session Security
Local transport endpoints (Unix domain sockets or Windows named pipes) are not automatically secure against local spoofing or unauthorized local connection:
1. **Windows:** The host calls `GetNamedPipeClientProcessId()` immediately upon connection to verify that the connecting client PID strictly matches the spawned child plugin process.
2. **Linux:** The host calls `getsockopt(socket_fd, SOL_SOCKET, SO_PEERCRED, &ucred, ...)` to cryptographically verify that `ucred.pid` and `ucred.uid` match the child process.
3. **Session-Key Derivation & Canonical HMAC Authentication:**
   - During initial process setup, the host generates a 256-bit cryptographically secure session key passed via an inherited private file descriptor (never via command-line arguments or environment variables).
   - **Canonical Message Authentication:** To prevent forgery, truncation, or field-injection attacks, every IPC message signs a canonical byte stream encompassing all protocol headers:
     `HMAC_SHA256(session_key, protocol_version || session_id || sequence_number || request_id || method_id || payload_len || payload)`
   - **Strict Replay & Ordering Protection:** Every message includes a strictly monotonic 64-bit `sequence_number`. The Capability Broker rejects any message with a sequence number $\le$ the highest observed sequence number for that session, neutralizing replay and packet re-ordering attacks.

### 7.2 Message Framing Protocol
Every IPC message begins with a fixed 32-byte header:
```text
+------------------+------------------+------------------+------------------+
| Magic (4 bytes)  | Version (2 bytes)| Msg Type (2 byte)| Request ID (4 B) |
+------------------+------------------+------------------+------------------+
| Payload Size (4B)| Reserved (4 B)   | HMAC-SHA256 Signature (16 bytes)    |
+------------------+------------------+-------------------------------------+
| Payload Bytes (Size bytes)                                                |
+---------------------------------------------------------------------------+
```

### 7.3 Shared Memory Safety Contracts
For streaming high-bandwidth data (e.g. video frames, audio buffers):
1. **Memory Ordering:** Control structures in shared memory use lock-free atomic sequence numbers. The writer updates payload data and executes `std::atomic_thread_fence(std::memory_order_release)` before updating the write pointer; the reader reads with `std::memory_order_acquire`.
2. **Integer-Overflow-Safe Bounds Checking:**
   - *Security Rule:* Direct evaluation of `offset + length <= total_mapped_size` is vulnerable to integer overflow attacks if malicious plugins pass large unsigned values (e.g., `UINT64_MAX - 10`).
   - The host enforces overflow-safe subtraction bounds checking:
     ```cpp
     bool is_safe_buffer_range(size_t offset, size_t length, size_t mapped_size) {
         return (offset <= mapped_size) && (length <= (mapped_size - offset));
     }
     ```
   - Requests failing this check are rejected immediately with `PD_PLUGIN_ERR_INVALID_ARGUMENT`.
3. **Generation IDs & Revocation:** Every mapped region has a monotonic `generation_id`. If a plugin crashes or terminates, the host immediately unmaps the memory (`UnmapViewOfFile` on Windows, `munmap` on Linux) and destroys the shared memory handle, permanently revoking access.

---

# 8. Crash Isolation, Watchdogs & Resilience

A major failure mode of native plugins is unhandled exceptions (null dereference, buffer overflow) crashing the host:

```text
Plugin Child Process crashes (SIGSEGV / Access Violation)
                 |
                 v
Host Watchdog intercepts process exit code (WaitPid / WaitForSingleObject)
                 |
+----------------v--------------------------------------+
|             Host Recovery Protocol                    |
|                                                       |
|  1. Reclaim all active buffers and locks              |
|  2. Invalidate pending RPC requests with Status::Lost |
|  3. Log crash report to telemetry                     |
|  4. Notify Application: "Camera plugin has crashed"   |
|  5. Apply exponential backoff restart (Max 3 / min)   |
+-------------------------------------------------------+
                 |
                 v
   Host UI continues running smoothly without freeze
```

---

# 9. Plugin Signing, Verification & Provenance

1. **Digital Signatures:**
   - Plugin package archives (`.pdp`) are signed using Ed25519 or X.509 certificates.
   - The archive contains a cryptographic manifest signature (`manifest.sig`) and payload digests.
2. **Trust Policies:**
   - **Enforced Mode (Production):** Plugins without a valid, trusted signature fail verification and are rejected.
   - **Developer Mode:** Unsigned local plugins require an explicit flag (`pd run --allow-unsigned-plugins`).

---

# 10. Core C++20 Plugin Host Interfaces

```cpp
// pingdart/plugins/plugin_manager.h
#pragma once
#include <pingdart/core/result.h>
#include <string>
#include <memory>
#include <vector>

namespace pingdart::plugins {

enum class IsolationMode {
    InProcess,
    SandboxedProcess
};

struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    IsolationMode isolation;
    std::vector<std::string> permissions;
    uint32_t memory_limit_mb{128};
};

class IPluginInstance {
public:
    virtual ~IPluginInstance() = default;

    [[nodiscard]] virtual const PluginDescriptor& descriptor() const noexcept = 0;
    [[nodiscard]] virtual bool is_alive() const noexcept = 0;

    virtual core::Result<std::vector<uint8_t>> invoke(
        const std::string& method,
        const std::vector<uint8_t>& payload) = 0;
};

class PluginManager {
public:
    virtual ~PluginManager() = default;

    virtual core::Status load_plugin(const std::string& plugin_directory) = 0;
    virtual void unload_plugin(const std::string& plugin_id) = 0;

    [[nodiscard]] virtual IPluginInstance* get_plugin(const std::string& plugin_id) = 0;

    static std::unique_ptr<PluginManager> create();
};

} // namespace pingdart::plugins
```

---

# 11. DOC-06 Acceptance Criteria

The Plugin SDK & Security Architecture is validated when the following test suite passes:
1. **ABI Loading:** A sample plugin compiles against `pd_plugin_abi.h` and loads successfully on both Windows and Linux.
2. **Capability Brokering:** Calling an authorized API succeeds; attempting an unauthorized API returns `PD_PLUGIN_ERR_PERMISSION_DENIED`.
3. **Crash Isolation:** Intentionally triggering a null-pointer dereference inside an out-of-process plugin terminates the child process cleanly; the host detects the exit and reports `StatusCode::Unavailable` without crashing.
4. **Clean Teardown:** Host shutdown safely destroys child processes and unmaps shared memory with zero resource leaks.
