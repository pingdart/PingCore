#include "wasm_runtime.h"

namespace pingdart::web {

WasmRuntime::WasmRuntime() = default;

WasmRuntime::~WasmRuntime() {
    shutdown();
}

void WasmRuntime::initialize() {
    initialized_ = true;
}

void WasmRuntime::shutdown() {
    initialized_ = false;
    root_.reset();
    action_handlers_.clear();
    scheduler_.reset();
}

void WasmRuntime::set_root_widget(std::unique_ptr<SemanticWidget> root) {
    root_ = std::move(root);
}

void WasmRuntime::register_action_handler(const std::string& action_id, ActionHandler handler) {
    if (handler) {
        action_handlers_[action_id] = std::move(handler);
    }
}

bool WasmRuntime::dispatch_dom_event(const std::string& target_pd_id,
                                    const std::string& event_type,
                                    const std::string& action_id) {
    if (!initialized_) {
        return false;
    }

    dispatch_count_++;

    // 1. Direct action handler lookup
    if (!action_id.empty()) {
        auto it = action_handlers_.find(action_id);
        if (it != action_handlers_.end()) {
            it->second(target_pd_id, event_type);
            return true;
        }
    }

    // 2. Fallback to target id lookup
    auto it = action_handlers_.find(target_pd_id);
    if (it != action_handlers_.end()) {
        it->second(target_pd_id, event_type);
        return true;
    }

    return false;
}

bool WasmRuntime::verify_hydration_identity(const std::string& server_root_id) const {
    if (!root_) return false;
    std::string client_root_id = make_deterministic_id("", 0, root_->key());
    return client_root_id == server_root_id;
}

void WasmRuntime::step(double timestamp_ms) {
    if (!initialized_) return;
    scheduler_.step_frame(timestamp_ms);
}

// C ABI Exports
extern "C" {

void* pd_wasm_runtime_create() {
    return new (std::nothrow) WasmRuntime();
}

void pd_wasm_runtime_destroy(void* handle) {
    delete static_cast<WasmRuntime*>(handle);
}

int pd_wasm_runtime_init(void* handle) {
    if (!handle) return 0;
    static_cast<WasmRuntime*>(handle)->initialize();
    return 1;
}

void pd_wasm_runtime_step(void* handle, double timestamp_ms) {
    if (!handle) return;
    static_cast<WasmRuntime*>(handle)->step(timestamp_ms);
}

int pd_wasm_runtime_dispatch_event(void* handle, const char* target_id, const char* event_type, const char* action_id) {
    if (!handle || !target_id || !event_type) return 0;
    std::string action = action_id ? action_id : "";
    bool ok = static_cast<WasmRuntime*>(handle)->dispatch_dom_event(target_id, event_type, action);
    return ok ? 1 : 0;
}

} // extern "C"

} // namespace pingdart::web
