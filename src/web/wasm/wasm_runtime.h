#pragma once

#include <pingdart/web/semantic_widget.h>
#include <pingdart/web/browser_scheduler.h>
#include "../browser/browser_scheduler.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace pingdart::web {

using ActionHandler = std::function<void(const std::string& target_id, const std::string& event_type)>;

class WasmRuntime final {
public:
    WasmRuntime();
    ~WasmRuntime();

    void initialize();
    void shutdown();

    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
    [[nodiscard]] BrowserScheduler& scheduler() noexcept { return scheduler_; }

    void set_root_widget(std::unique_ptr<SemanticWidget> root);
    [[nodiscard]] const SemanticWidget* root_widget() const noexcept { return root_.get(); }

    // Registers a client action handler triggered by a DOM event
    void register_action_handler(const std::string& action_id, ActionHandler handler);

    // Dispatches an event from the browser DOM (e.g. click on button with data-pd-id)
    bool dispatch_dom_event(const std::string& target_pd_id,
                           const std::string& event_type,
                           const std::string& action_id = "");

    // Hydration handshake: verifies element tree matches server-rendered deterministic IDs
    [[nodiscard]] bool verify_hydration_identity(const std::string& server_root_id) const;

    // Turn execution: single non-blocking step for browser requestAnimationFrame
    void step(double timestamp_ms);

    [[nodiscard]] uint32_t dispatch_count() const noexcept { return dispatch_count_; }

private:
    bool initialized_{false};
    BrowserScheduler scheduler_;
    std::unique_ptr<SemanticWidget> root_;
    std::unordered_map<std::string, ActionHandler> action_handlers_;
    uint32_t dispatch_count_{0};
};

// C ABI Exports for WebAssembly / JS integration
extern "C" {
    void* pd_wasm_runtime_create();
    void pd_wasm_runtime_destroy(void* handle);
    int pd_wasm_runtime_init(void* handle);
    void pd_wasm_runtime_step(void* handle, double timestamp_ms);
    int pd_wasm_runtime_dispatch_event(void* handle, const char* target_id, const char* event_type, const char* action_id);
}

} // namespace pingdart::web
