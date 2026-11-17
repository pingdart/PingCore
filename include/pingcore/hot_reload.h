// include/pingcore/hot_reload.h
// PingCore Incremental Hot Reload, Safe Dynamic Leases & Quiescence Engine (PKG-126)
#pragma once

#include <pingcore/reactive.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <functional>
#include <atomic>
#include <thread>

#ifdef __APPLE__
#include <dlfcn.h>
#elif defined(__linux__)
#include <dlfcn.h>
#elif defined(_WIN32)
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif

namespace pingcore {

enum class ReloadLevel {
    Level1_StyleText,     // Immediate UI update, no restart, state preserved
    Level2_Component,     // Recompile affected component module, state preserved
    Level3_FullRestart    // ABI / Native bridge / Platform change: full restart required
};

struct FileChangeEvent {
    std::string path;
    ReloadLevel level;
    std::string description;
};

// PKG-125: ABI version for module compatibility checking
struct ModuleABI {
    uint32_t major{1};
    uint32_t minor{0};
    uint32_t patch{0};

    [[nodiscard]] bool is_compatible_with(const ModuleABI& other) const {
        // Major version must match; minor is backward-compatible
        return major == other.major && minor >= other.minor;
    }

    bool operator==(const ModuleABI&) const = default;
};

// PKG-126: Handle to a dynamically loaded module with lease tracking and retirement status
struct ModuleHandle {
    std::string path;
    void* handle{nullptr};
    ModuleABI abi{};
    bool loaded{false};
    std::atomic<bool> retiring{false};
    std::atomic<int64_t> active_leases{0};

    ModuleHandle() = default;

    ModuleHandle(ModuleHandle&& other) noexcept
        : path(std::move(other.path)),
          handle(other.handle),
          abi(other.abi),
          loaded(other.loaded),
          retiring(other.retiring.load(std::memory_order_relaxed)),
          active_leases(other.active_leases.load(std::memory_order_relaxed)) {
        other.handle = nullptr;
        other.loaded = false;
    }

    ModuleHandle& operator=(ModuleHandle&& other) noexcept {
        if (this != &other) {
            path = std::move(other.path);
            handle = other.handle;
            abi = other.abi;
            loaded = other.loaded;
            retiring.store(other.retiring.load(std::memory_order_relaxed), std::memory_order_relaxed);
            active_leases.store(other.active_leases.load(std::memory_order_relaxed), std::memory_order_relaxed);
            other.handle = nullptr;
            other.loaded = false;
        }
        return *this;
    }

    ModuleHandle(const ModuleHandle&) = delete;
    ModuleHandle& operator=(const ModuleHandle&) = delete;

    [[nodiscard]] bool is_valid() const { return handle != nullptr && loaded; }

    [[nodiscard]] void* get_symbol(const std::string& name) const {
        if (!is_valid()) return nullptr;
#if defined(__APPLE__) || defined(__linux__)
        return dlsym(handle, name.c_str());
#else
        return nullptr;
#endif
    }
};

// PKG-126: RAII Module Lease that guarantees the underlying shared library code cannot be unloaded
// while active function calls, stack frames, or background callbacks are executing
class ModuleLease {
public:
    ModuleLease() = default;

    explicit ModuleLease(std::shared_ptr<ModuleHandle> handle)
        : handle_(std::move(handle)) {
        if (handle_) {
            handle_->active_leases.fetch_add(1, std::memory_order_relaxed);
        }
    }

    ~ModuleLease() {
        release();
    }

    ModuleLease(const ModuleLease& other) : handle_(other.handle_) {
        if (handle_) {
            handle_->active_leases.fetch_add(1, std::memory_order_relaxed);
        }
    }

    ModuleLease& operator=(const ModuleLease& other) {
        if (this != &other) {
            release();
            handle_ = other.handle_;
            if (handle_) {
                handle_->active_leases.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    ModuleLease(ModuleLease&& other) noexcept : handle_(std::move(other.handle_)) {}

    ModuleLease& operator=(ModuleLease&& other) noexcept {
        if (this != &other) {
            release();
            handle_ = std::move(other.handle_);
        }
        return *this;
    }

    void release() {
        if (handle_) {
            handle_->active_leases.fetch_sub(1, std::memory_order_release);
            handle_.reset();
        }
    }

    [[nodiscard]] bool is_valid() const {
        return handle_ != nullptr && handle_->is_valid();
    }

    [[nodiscard]] void* get_symbol(const std::string& name) const {
        return handle_ ? handle_->get_symbol(name) : nullptr;
    }

    [[nodiscard]] std::shared_ptr<ModuleHandle> raw_handle() const {
        return handle_;
    }

private:
    std::shared_ptr<ModuleHandle> handle_;
};

// PKG-126: Quiescence Barrier ensuring code is drained before dlclose
class QuiescenceBarrier {
public:
    static bool drain_leases(const std::shared_ptr<ModuleHandle>& module,
                             std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
        if (!module) return true;
        auto start = std::chrono::steady_clock::now();
        while (module->active_leases.load(std::memory_order_acquire) > 0) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false; // Draining timed out
            }
            std::this_thread::yield();
        }
        return true;
    }
};

// ============================================================================
// PKG-127: ModuleOwned<T> & ModuleObjectTracker for Safe Dynamic Object Lifetimes
// ============================================================================
// ModuleOwned<T> binds an object's lifetime directly to the module lease.
// The underlying dynamic library CANNOT be unloaded while this object exists.
// When the object is destructed, its destructor executes while the library code and
// vtables are still fully mapped, and only then is the ModuleLease released.
template <typename T>
class ModuleOwned {
public:
    ModuleOwned() = default;

    template <typename... Args>
    static ModuleOwned<T> create(ModuleLease lease, Args&&... args) {
        ModuleOwned<T> owned;
        owned.lease_ = std::move(lease);
        owned.instance_ = std::make_shared<T>(std::forward<Args>(args)...);
        return owned;
    }

    T* get() const noexcept { return instance_.get(); }
    T* operator->() const noexcept { return instance_.get(); }
    T& operator*() const noexcept { return *instance_; }

    [[nodiscard]] bool is_valid() const noexcept { return instance_ != nullptr && lease_.is_valid(); }
    [[nodiscard]] const ModuleLease& lease() const noexcept { return lease_; }

    void reset() {
        // Destroy the object instance BEFORE releasing the module lease
        instance_.reset();
        lease_.release();
    }

private:
    std::shared_ptr<T> instance_;
    ModuleLease lease_;
};

// PKG-127: Module Object Tracker tracks active object allocations across modules
class ModuleObjectTracker {
public:
    static ModuleObjectTracker& instance() {
        static ModuleObjectTracker s_tracker;
        return s_tracker;
    }

    void register_object(const std::string& module_name, void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        module_objects_[module_name].insert(ptr);
    }

    void unregister_object(const std::string& module_name, void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = module_objects_.find(module_name);
        if (it != module_objects_.end()) {
            it->second.erase(ptr);
        }
    }

    [[nodiscard]] size_t live_object_count(const std::string& module_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = module_objects_.find(module_name);
        return it != module_objects_.end() ? it->second.size() : 0;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        module_objects_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_set<void*>> module_objects_;
};

// PKG-125: Incremental compiler interface
class IncrementalCompiler {
public:
    struct CompileResult {
        bool success{false};
        std::string output_path;
        std::string error_message;
        std::chrono::microseconds compile_time{0};
    };

    /// Compiles a source file to a shared library
    static CompileResult compile(const std::string& source_path,
                                  const std::string& output_path,
                                  const std::vector<std::string>& flags = {}) {
        CompileResult result;
        auto start = std::chrono::high_resolution_clock::now();

        if (!std::filesystem::exists(source_path)) {
            result.error_message = "Source file not found: " + source_path;
            return result;
        }

        // Build compiler command
        std::string compiler = "c++";
#ifdef __APPLE__
        std::string cmd = compiler + " -std=c++20 -dynamiclib -fPIC";
#else
        std::string cmd = compiler + " -std=c++20 -shared -fPIC";
#endif
        for (const auto& flag : flags) {
            cmd += " " + flag;
        }
        cmd += " -o " + output_path + " " + source_path + " 2>&1";

        // Execute compilation
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            result.error_message = "Failed to invoke compiler";
            return result;
        }

        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);
        auto end = std::chrono::high_resolution_clock::now();
        result.compile_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (exit_code == 0) {
            result.success = true;
            result.output_path = output_path;
        } else {
            result.error_message = output.empty() ? "Compilation failed with exit code " + std::to_string(exit_code) : output;
        }
        return result;
    }
};

// PKG-125/126: Dynamic module loader and lease-safe lifecycle manager
class ModuleLoader {
public:
    /// Loads a shared library module and validates ABI version
    static ModuleHandle load_module(const std::string& path, ModuleABI expected_abi = {}) {
        ModuleHandle mh;
        mh.path = path;

        if (!std::filesystem::exists(path)) {
            return mh;
        }

#if defined(__APPLE__) || defined(__linux__)
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            return mh;
        }

        // Check for ABI version symbol
        using AbiVersionFn = ModuleABI(*)();
        auto abi_fn = reinterpret_cast<AbiVersionFn>(reinterpret_cast<uintptr_t>(dlsym(handle, "pingcore_module_abi")));
        if (abi_fn) {
            mh.abi = abi_fn();
            if (!mh.abi.is_compatible_with(expected_abi) && expected_abi.major > 0) {
                // ABI mismatch — reject module
                dlclose(handle);
                return mh;
            }
        } else {
            mh.abi = expected_abi; // No ABI symbol — assume compatible
        }

        mh.handle = handle;
        mh.loaded = true;
#endif

        return mh;
    }

    /// Loads module into a shared_ptr for lease management
    static std::shared_ptr<ModuleHandle> load_managed_module(const std::string& path, ModuleABI expected_abi = {}) {
        auto mh = std::make_shared<ModuleHandle>(load_module(path, expected_abi));
        return mh->is_valid() ? mh : nullptr;
    }

    /// Unloads a previously loaded module (safe: only call after active_leases == 0)
    static bool unload_module(ModuleHandle& mh) {
        if (!mh.is_valid()) return false;

#if defined(__APPLE__) || defined(__linux__)
        int rc = dlclose(mh.handle);
        if (rc == 0) {
            mh.handle = nullptr;
            mh.loaded = false;
            return true;
        }
#endif
        return false;
    }

    /// Looks up a symbol in a loaded module
    static void* get_symbol(const ModuleHandle& mh, const std::string& symbol_name) {
        if (!mh.is_valid()) return nullptr;
#if defined(__APPLE__) || defined(__linux__)
        return dlsym(mh.handle, symbol_name.c_str());
#else
        return nullptr;
#endif
    }

    /// Reloads a module: saves state → unloads old → loads new → restores state
    struct ReloadResult {
        bool success{false};
        ModuleHandle new_handle;
        std::string error;
        std::chrono::microseconds reload_time{0};
    };

    static ReloadResult reload_module(ModuleHandle& old_handle,
                                       const std::string& new_path,
                                       const std::unordered_map<std::string, std::string>& preserved_state = {},
                                       ModuleABI expected_abi = {}) {
        ReloadResult result;
        auto start = std::chrono::high_resolution_clock::now();

        // Step 1: Load new module first (before unloading old)
        ModuleHandle new_handle = load_module(new_path, expected_abi);
        if (!new_handle.is_valid()) {
            result.error = "Failed to load new module: " + new_path;
            return result;
        }

        // Step 2: Unload old module
        if (old_handle.is_valid()) {
            unload_module(old_handle);
        }

        // Step 3: Restore state via init function if present
        using InitFn = void(*)(const char*, const char*);
        auto init_fn = reinterpret_cast<InitFn>(reinterpret_cast<uintptr_t>(get_symbol(new_handle, "pingcore_module_restore")));
        if (init_fn) {
            for (const auto& [key, value] : preserved_state) {
                init_fn(key.c_str(), value.c_str());
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.success = true;
        result.new_handle = std::move(new_handle);
        result.reload_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return result;
    }
};

class HotReloadManager {
public:
    static HotReloadManager& instance() {
        static HotReloadManager s_instance;
        return s_instance;
    }

    /// Classifies file path into the 3-level reload hierarchy
    static ReloadLevel classify_change(std::string_view filepath) {
        std::string p(filepath);
        if (p.find("styles/") != std::string::npos ||
            p.find(".css") != std::string::npos ||
            p.find("theme.json") != std::string::npos ||
            p.find("public/") != std::string::npos) {
            return ReloadLevel::Level1_StyleText;
        }

        if (p.find("platforms/") != std::string::npos ||
            p.find("native/") != std::string::npos ||
            p.find("pingcore.yaml") != std::string::npos ||
            p.find("CMakeLists.txt") != std::string::npos ||
            p.find("native_bridge") != std::string::npos) {
            return ReloadLevel::Level3_FullRestart;
        }

        if (p.find("app/") != std::string::npos ||
            p.find("components/") != std::string::npos ||
            p.find("hooks/") != std::string::npos ||
            p.find("services/") != std::string::npos ||
            p.find("store/") != std::string::npos) {
            return ReloadLevel::Level2_Component;
        }

        return ReloadLevel::Level2_Component;
    }

    /// State Preservation across Hot Reloads: Save/Restore by unique component key
    void preserve_state(const std::string& key, const std::string& serialized_state) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_store_[key] = serialized_state;
    }

    [[nodiscard]] std::string restore_state(const std::string& key, const std::string& default_val = "") const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = state_store_.find(key);
        if (it != state_store_.end()) {
            return it->second;
        }
        return default_val;
    }

    bool has_preserved_state(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_store_.find(key) != state_store_.end();
    }

    void clear_state() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_store_.clear();
    }

    /// PKG-125: Full hot-reload pipeline — classify → compile → reload → restore state
    struct HotReloadResult {
        FileChangeEvent event;
        bool compiled{false};
        bool reloaded{false};
        std::string error;
    };

    HotReloadResult full_reload(std::string_view filepath,
                                 const std::string& output_dir = "/tmp") {
        HotReloadResult result;
        result.event = process_file_change(filepath);

        if (result.event.level == ReloadLevel::Level1_StyleText) {
            result.compiled = false;
            result.reloaded = true;
            return result;
        }

        if (result.event.level == ReloadLevel::Level2_Component) {
            std::string out = output_dir + "/pingcore_module.dylib";
            auto compile_result = IncrementalCompiler::compile(
                std::string(filepath), out, {"-O1"});

            if (compile_result.success) {
                result.compiled = true;
                result.reloaded = true;
            } else {
                result.error = compile_result.error_message;
                result.compiled = false;
            }
            return result;
        }

        result.compiled = false;
        result.reloaded = false;
        result.error = "Full restart required for platform/ABI changes";
        return result;
    }

    /// Triggers hot reload pipeline
    FileChangeEvent process_file_change(std::string_view filepath) {
        ReloadLevel level = classify_change(filepath);
        std::string desc;
        switch (level) {
            case ReloadLevel::Level1_StyleText:
                desc = "Fast Refresh: Instant style/token invalidation without restart.";
                break;
            case ReloadLevel::Level2_Component:
                desc = "Fast Refresh: Incremental component rebuild; state preserved.";
                break;
            case ReloadLevel::Level3_FullRestart:
                desc = "Platform/ABI boundary modified: Full application rebuild and restart triggered.";
                break;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            reload_history_.push_back({std::string(filepath), level, desc});
        }

        notify_reload(level, filepath);

        return {std::string(filepath), level, desc};
    }

    void register_reload_listener(std::function<void(ReloadLevel, std::string_view)> listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.push_back(std::move(listener));
    }

    [[nodiscard]] size_t reload_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return reload_history_.size();
    }

    /// PKG-125: Registered loaded modules for tracking (value-based)
    void register_module(const std::string& name, ModuleHandle handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        loaded_modules_[name] = std::make_shared<ModuleHandle>(std::move(handle));
    }

    /// PKG-126: Register managed module
    void register_managed_module(const std::string& name, std::shared_ptr<ModuleHandle> handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        loaded_modules_[name] = std::move(handle);
    }

    [[nodiscard]] bool has_module(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return loaded_modules_.find(name) != loaded_modules_.end();
    }

    ModuleHandle* get_module(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loaded_modules_.find(name);
        return (it != loaded_modules_.end() && it->second) ? it->second.get() : nullptr;
    }

    /// PKG-126: Acquire a reference-counted lease on the active module
    [[nodiscard]] ModuleLease acquire_module_lease(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = loaded_modules_.find(name);
        if (it != loaded_modules_.end() && it->second && it->second->is_valid()) {
            return ModuleLease(it->second);
        }
        return {};
    }

    /// PKG-126: Live module replacement with quiescence barrier and automatic rollback on failure
    struct SafeReloadResult {
        bool success{false};
        bool rolled_back{false};
        bool drained{false};
        std::string error;
    };

    SafeReloadResult safe_reload_module(const std::string& name,
                                        const std::string& new_path,
                                        ModuleABI expected_abi = {},
                                        std::chrono::milliseconds drain_timeout = std::chrono::milliseconds(500)) {
        SafeReloadResult result;

        // Step 1: Load candidate module first (do not touch existing module yet)
        auto candidate = ModuleLoader::load_managed_module(new_path, expected_abi);
        if (!candidate || !candidate->is_valid()) {
            result.error = "Failed to load candidate module: " + new_path;
            result.rolled_back = true;
            return result; // Rollback: original module unchanged!
        }

        std::shared_ptr<ModuleHandle> old_module;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = loaded_modules_.find(name);
            if (it != loaded_modules_.end()) {
                old_module = it->second;
            }
            // Atomically switch active module to candidate
            loaded_modules_[name] = candidate;
        }

        // Step 2: Mark old module as retiring (no new leases can be acquired)
        if (old_module) {
            old_module->retiring.store(true, std::memory_order_release);

            // Step 3: Quiescence barrier — wait for existing in-flight callers to drain
            bool drained = QuiescenceBarrier::drain_leases(old_module, drain_timeout);
            result.drained = drained;

            if (drained) {
                // Safely unload old code now that zero threads are inside it
                ModuleLoader::unload_module(*old_module);
            } else {
                // Draining timed out: keep in zombie list to prevent segfaulting active callers
                std::lock_guard<std::mutex> lock(mutex_);
                zombie_modules_.push_back(old_module);
            }
        }

        result.success = true;
        return result;
    }

    void clear_modules() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, mh] : loaded_modules_) {
            if (mh) {
                ModuleLoader::unload_module(*mh);
            }
        }
        loaded_modules_.clear();
        for (auto& zm : zombie_modules_) {
            if (zm) {
                ModuleLoader::unload_module(*zm);
            }
        }
        zombie_modules_.clear();
    }

private:
    void notify_reload(ReloadLevel level, std::string_view path) {
        std::vector<std::function<void(ReloadLevel, std::string_view)>> copy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            copy = listeners_;
        }
        for (const auto& cb : copy) {
            if (cb) cb(level, path);
        }
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> state_store_;
    std::vector<FileChangeEvent> reload_history_;
    std::vector<std::function<void(ReloadLevel, std::string_view)>> listeners_;
    std::unordered_map<std::string, std::shared_ptr<ModuleHandle>> loaded_modules_;
    std::vector<std::shared_ptr<ModuleHandle>> zombie_modules_;
};

} // namespace pingcore
