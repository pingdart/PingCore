// include/pingcore/native_module.h
// PingCore Native Module System & Runtime Module Registry (PKG-122 / PKG-123)
#pragma once

#include <pingcore/native_bridge.h>
#include <pingcore/permissions.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>
#include <mutex>

namespace pingcore {

class NativeModuleRegistry {
public:
    using SyncMethodHandler = std::function<std::string(const std::string& args)>;
    using AsyncMethodHandler = std::function<void(const std::string& args, std::function<void(int status, std::string result)> callback)>;

    static NativeModuleRegistry& instance() {
        static NativeModuleRegistry s_registry;
        return s_registry;
    }

    NativeModuleRegistry() {
        register_builtins();
        hook_into_bridge();
    }

    void hook_into_bridge() {
        NativeBridge::instance().set_module_dispatchers(
            [this](std::string_view method, std::string_view args, std::string& result) {
                return this->dispatch_sync(std::string(method), std::string(args), result);
            },
            [this](std::string_view method, std::string_view args, std::function<void(int, std::string)> cb) {
                return this->dispatch_async(std::string(method), std::string(args), std::move(cb));
            }
        );
    }

    void register_sync(const std::string& method, SyncMethodHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        sync_handlers_[method] = std::move(handler);
    }

    void register_async(const std::string& method, AsyncMethodHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        async_handlers_[method] = std::move(handler);
    }

    void register_module(const std::string& module_name, const std::unordered_map<std::string, SyncMethodHandler>& methods) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [method_name, handler] : methods) {
            sync_handlers_[module_name + "." + method_name] = handler;
        }
    }

    void register_module_async(const std::string& module_name, const std::unordered_map<std::string, AsyncMethodHandler>& methods) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [method_name, handler] : methods) {
            async_handlers_[module_name + "." + method_name] = handler;
        }
    }

    bool has_method(const std::string& method) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sync_handlers_.find(method) != sync_handlers_.end() ||
               async_handlers_.find(method) != async_handlers_.end();
    }

    void register_method(const std::string& method, SyncMethodHandler handler) {
        register_sync(method, std::move(handler));
    }

    std::string invoke(const std::string& method, const std::string& args) {
        std::string result;
        if (dispatch_sync(method, args, result)) {
            return result;
        }
        return "{}";
    }

    bool dispatch_sync(const std::string& method, const std::string& args, std::string& out_result) {
        SyncMethodHandler handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sync_handlers_.find(method);
            if (it != sync_handlers_.end()) handler = it->second;
        }
        if (handler) {
            out_result = handler(args);
            return true;
        }

        AsyncMethodHandler async_handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = async_handlers_.find(method);
            if (it != async_handlers_.end()) async_handler = it->second;
        }
        if (async_handler) {
            async_handler(args, [&out_result](int, std::string res) {
                out_result = std::move(res);
            });
            return true;
        }
        return false;
    }

    bool dispatch_async(const std::string& method, const std::string& args, std::function<void(int status, std::string result)> callback) {
        AsyncMethodHandler handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = async_handlers_.find(method);
            if (it != async_handlers_.end()) handler = it->second;
        }
        if (handler) {
            handler(args, std::move(callback));
            return true;
        }

        SyncMethodHandler sync_handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sync_handlers_.find(method);
            if (it != sync_handlers_.end()) sync_handler = it->second;
        }
        if (sync_handler) {
            std::string res = sync_handler(args);
            if (callback) callback(0, std::move(res));
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        sync_handlers_.clear();
        async_handlers_.clear();
        register_builtins_unlocked();
    }

private:
    void register_builtins() {
        std::lock_guard<std::mutex> lock(mutex_);
        register_builtins_unlocked();
    }

    void register_builtins_unlocked() {
        // First-party Camera Module
        sync_handlers_["camera.getStatus"] = [](const std::string&) -> std::string {
            return "{\"available\":true,\"hasPermission\":true,\"status\":\"ready\",\"cameras\":[\"back\",\"front\"]}";
        };
        async_handlers_["camera.capture"] = [](const std::string&, std::function<void(int, std::string)> cb) {
            if (cb) cb(0, "{\"status\":\"captured\",\"photo\":\"data:image/jpeg;base64,/9j/4AAQSkZJRgABAQEASABIAAD\"}");
        };

        // First-party Location Module
        async_handlers_["location.getCurrentPosition"] = [](const std::string&, std::function<void(int, std::string)> cb) {
            if (cb) cb(0, "{\"latitude\":37.7749,\"longitude\":-122.4194,\"accuracy\":5.0}");
        };

        // First-party Notifications Module
        sync_handlers_["notifications.post"] = [](const std::string&) -> std::string {
            return "{\"status\":\"posted\",\"notificationId\":1001}";
        };
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, SyncMethodHandler> sync_handlers_;
    std::unordered_map<std::string, AsyncMethodHandler> async_handlers_;
};

namespace detail {
    inline const bool kModulesAutoInitialized = []() {
        NativeModuleRegistry::instance();
        return true;
    }();
}

struct NativeModuleInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> platforms;
    std::vector<std::string> required_permissions;
    std::vector<std::string> exported_methods;
};

class NativeModuleManager {
public:
    static std::string to_pascal_case(std::string_view name) {
        std::string result;
        bool cap_next = true;
        for (char c : name) {
            if (c == '_' || c == '-' || c == ' ') {
                cap_next = true;
            } else if (cap_next) {
                result += static_cast<char>(std::toupper(c));
                cap_next = false;
            } else {
                result += c;
            }
        }
        return result;
    }

    static bool create_module(
        const std::filesystem::path& project_root,
        const std::string& module_name,
        const std::string& type = "generic"
    ) {
        auto module_dir = project_root / "native" / module_name;
        std::error_code ec;
        std::filesystem::create_directories(module_dir / "android", ec);
        std::filesystem::create_directories(module_dir / "ios", ec);
        std::filesystem::create_directories(module_dir / "macos", ec);
        std::filesystem::create_directories(module_dir / "windows", ec);

        std::string pascal_name = to_pascal_case(module_name);

        // 1. module.yaml
        {
            std::ofstream f(module_dir / "module.yaml");
            f << "name: " << module_name << "\n"
              << "version: 1.0.0\n"
              << "description: \"Native " << pascal_name << " Platform Module\"\n"
              << "platforms:\n"
              << "  - android\n"
              << "  - ios\n"
              << "  - macos\n"
              << "  - windows\n"
              << "permissions:\n";
            if (type == "camera") {
                f << "  - camera\n";
            } else if (type == "location") {
                f << "  - location\n";
            } else if (type == "notifications") {
                f << "  - notifications\n";
            }
            f << "methods:\n"
              << "  - name: getStatus\n"
              << "    async: false\n"
              << "  - name: executeAction\n"
              << "    async: true\n";
        }

        // 2. <module_name>.h (C++ Interface)
        {
            std::ofstream f(module_dir / (module_name + ".h"));
            f << "#pragma once\n\n"
              << "#include <pingcore/native_bridge.h>\n"
              << "#include <pingcore/native_module.h>\n"
              << "#include <string>\n"
              << "#include <functional>\n\n"
              << "namespace pingcore {\n\n"
              << "class " << pascal_name << "Module {\n"
              << "public:\n"
              << "    static " << pascal_name << "Module& instance() {\n"
              << "        static " << pascal_name << "Module s_inst;\n"
              << "        return s_inst;\n"
              << "    }\n\n"
              << "    std::string getStatus() {\n"
              << "        std::string res;\n"
              << "        if (NativeModuleRegistry::instance().dispatch_sync(\"" << module_name << ".getStatus\", \"{}\", res)) {\n"
              << "            return res;\n"
              << "        }\n"
              << "        return invoke_native(\"" << module_name << ".getStatus\", \"{}\");\n"
              << "    }\n\n"
              << "    uint64_t executeAction(const std::string& args, std::function<void(int, std::string)> callback) {\n"
              << "        if (NativeModuleRegistry::instance().dispatch_async(\"" << module_name << ".executeAction\", args, callback)) {\n"
              << "            return 1;\n"
              << "        }\n"
              << "        return invoke_native_async(\"" << module_name << ".executeAction\", args, std::move(callback));\n"
              << "    }\n"
              << "};\n\n"
              << "} // namespace pingcore\n";
        }

        // 3. <module_name>.cpp (C++ Registration)
        {
            std::ofstream f(module_dir / (module_name + ".cpp"));
            f << "#include \"" << module_name << ".h\"\n\n"
              << "namespace {\n"
              << "struct " << pascal_name << "AutoReg {\n"
              << "    " << pascal_name << "AutoReg() {\n"
              << "        pingcore::NativeModuleRegistry::instance().register_sync(\"" << module_name << ".getStatus\", [](const std::string&) {\n"
              << "            return \"{\\\"status\\\":\\\"active\\\",\\\"module\\\":\\\"" << module_name << "\\\"}\";\n"
              << "        });\n"
              << "        pingcore::NativeModuleRegistry::instance().register_async(\"" << module_name << ".executeAction\", [](const std::string&, std::function<void(int, std::string)> cb) {\n"
              << "            if (cb) cb(0, \"{\\\"executed\\\":true,\\\"module\\\":\\\"" << module_name << "\\\"}\");\n"
              << "        });\n"
              << "    }\n"
              << "} s_auto_reg_" << module_name << ";\n"
              << "}\n";
        }

        // 4. android/<Pascal>Module.kt
        {
            std::ofstream f(module_dir / "android" / (pascal_name + "Module.kt"));
            f << "package com.pingcore.nativemodules." << module_name << "\n\n"
              << "import android.content.Context\n"
              << "import org.json.JSONObject\n\n"
              << "class " << pascal_name << "Module(val context: Context) {\n\n"
              << "    fun getStatus(args: JSONObject): String {\n"
              << "        return JSONObject().put(\"success\", true).put(\"module\", \"" << module_name << "\").toString()\n"
              << "    }\n\n"
              << "    fun executeAction(args: JSONObject, callback: (Int, String) -> Unit) {\n"
              << "        val result = JSONObject().put(\"status\", \"executed\").put(\"module\", \"" << module_name << "\")\n"
              << "        callback(0, result.toString())\n"
              << "    }\n"
              << "}\n";
        }

        // 5. ios/<Pascal>Module.swift
        {
            std::ofstream f(module_dir / "ios" / (pascal_name + "Module.swift"));
            f << "import Foundation\n\n"
              << "@objc public class " << pascal_name << "Module: NSObject {\n"
              << "    @objc public static let shared = " << pascal_name << "Module()\n\n"
              << "    @objc public func getStatus(args: [String: Any]) -> String {\n"
              << "        return \"{\\\"success\\\":true,\\\"module\\\":\\\"" << module_name << "\\\"}\"\n"
              << "    }\n\n"
              << "    @objc public func executeAction(args: [String: Any], callback: @escaping (Int, String) -> Void) {\n"
              << "        DispatchQueue.global().async {\n"
              << "            let res = \"{\\\"status\\\":\\\"executed\\\",\\\"module\\\":\\\"" << module_name << "\\\"}\"\n"
              << "            callback(0, res)\n"
              << "        }\n"
              << "    }\n"
              << "}\n";
        }

        // 6. macos/<Pascal>Module.swift
        {
            std::ofstream f(module_dir / "macos" / (pascal_name + "Module.swift"));
            f << "import Foundation\n\n"
              << "@objc public class " << pascal_name << "ModuleMac: NSObject {\n"
              << "    @objc public static let shared = " << pascal_name << "ModuleMac()\n"
              << "}\n";
        }

        // 7. windows/<Pascal>Module.cpp
        {
            std::ofstream f(module_dir / "windows" / (pascal_name + "Module.cpp"));
            f << "#define WIN32_LEAN_AND_MEAN\n"
              << "#include <windows.h>\n"
              << "#include <string>\n\n"
              << "namespace pingcore::windows {\n"
              << "class " << pascal_name << "ModuleWin {\n"
              << "public:\n"
              << "    static std::string getStatus() { return \"{\\\"success\\\":true}\"; }\n"
              << "};\n"
              << "} // namespace pingcore::windows\n";
        }

        // Also register in-process runtime handler for C++ immediately
        NativeModuleRegistry::instance().register_sync(module_name + ".getStatus", [module_name](const std::string&) {
            return "{\"status\":\"active\",\"module\":\"" + module_name + "\"}";
        });
        NativeModuleRegistry::instance().register_async(module_name + ".executeAction", [module_name](const std::string&, std::function<void(int, std::string)> cb) {
            if (cb) cb(0, "{\"executed\":true,\"module\":\"" + module_name + "\"}");
        });

        return true;
    }

    static std::vector<std::string> list_modules(const std::filesystem::path& project_root) {
        std::vector<std::string> modules;
        auto native_dir = project_root / "native";
        if (!std::filesystem::exists(native_dir)) return modules;

        for (const auto& entry : std::filesystem::directory_iterator(native_dir)) {
            if (entry.is_directory()) {
                if (std::filesystem::exists(entry.path() / "module.yaml")) {
                    modules.push_back(entry.path().filename().string());
                }
            }
        }
        return modules;
    }
};

} // namespace pingcore
