// include/pingcore/permissions.h
// PingCore Unified Version-Aware Permission Registry & Manifest Injector (PKG-121 / PKG-122 / PKG-123)
#pragma once

#include <pingcore/native_bridge.h>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <map>
#include <mutex>

namespace pingcore {

struct AndroidPermissionEntry {
    std::string name;
    int min_api{0};
    int max_api{0};
    std::string flags;
    std::string feature_req;
};

struct BluetoothOptions {
    bool connect{true};
    bool scan{true};
    bool advertise{false};
    bool never_for_location{true};
};

struct PermissionDescriptor {
    PermissionType type;
    std::string key_name;
    bool enabled{false};
    std::string reason;
    std::vector<AndroidPermissionEntry> android_entries;
    std::vector<std::string> ios_plist_keys;
    BluetoothOptions bluetooth_opts;
};

class PermissionManager {
public:
    static PermissionDescriptor get_descriptor(PermissionType type) {
        switch (type) {
            case PermissionType::Camera:
                return {
                    PermissionType::Camera,
                    "camera",
                    true,
                    "Camera access is required for scanning codes and capturing media.",
                    {
                        {"android.permission.CAMERA", 0, 0, "", "android.hardware.camera.any"}
                    },
                    {"NSCameraUsageDescription"},
                    {}
                };
            case PermissionType::Microphone:
                return {
                    PermissionType::Microphone,
                    "microphone",
                    true,
                    "Microphone access is required for voice recording and calls.",
                    {
                        {"android.permission.RECORD_AUDIO", 0, 0, "", "android.hardware.microphone"}
                    },
                    {"NSMicrophoneUsageDescription"},
                    {}
                };
            case PermissionType::Location:
                return {
                    PermissionType::Location,
                    "location",
                    true,
                    "Location access is required to discover nearby services.",
                    {
                        {"android.permission.ACCESS_FINE_LOCATION", 0, 0, "", ""},
                        {"android.permission.ACCESS_COARSE_LOCATION", 0, 0, "", ""},
                        {"android.permission.ACCESS_BACKGROUND_LOCATION", 29, 0, "", ""}
                    },
                    {"NSLocationWhenInUseUsageDescription", "NSLocationAlwaysAndWhenInUseUsageDescription"},
                    {}
                };
            case PermissionType::Notifications:
                return {
                    PermissionType::Notifications,
                    "notifications",
                    true,
                    "Notifications keep you updated with real-time alerts.",
                    {
                        {"android.permission.POST_NOTIFICATIONS", 33, 0, "", ""}
                    },
                    {},
                    {}
                };
            case PermissionType::Storage:
                return {
                    PermissionType::Storage,
                    "storage",
                    true,
                    "Storage access is required to save and read media files.",
                    {
                        {"android.permission.READ_MEDIA_IMAGES", 33, 0, "", ""},
                        {"android.permission.READ_MEDIA_VIDEO", 33, 0, "", ""},
                        {"android.permission.READ_MEDIA_AUDIO", 33, 0, "", ""},
                        {"android.permission.READ_EXTERNAL_STORAGE", 0, 32, "", ""},
                        {"android.permission.WRITE_EXTERNAL_STORAGE", 0, 29, "", ""}
                    },
                    {"NSPhotoLibraryUsageDescription"},
                    {}
                };
            case PermissionType::Bluetooth: {
                BluetoothOptions b_opts;
                b_opts.connect = true;
                b_opts.scan = true;
                b_opts.advertise = false;
                b_opts.never_for_location = true;
                return {
                    PermissionType::Bluetooth,
                    "bluetooth",
                    true,
                    "Bluetooth is required to connect to external hardware devices.",
                    {},
                    {"NSBluetoothAlwaysUsageDescription"},
                    b_opts
                };
            }
        }
        return {PermissionType::Camera, "camera", false, "", {}, {}, {}};
    }

    static PermissionDescriptor get_bluetooth_descriptor(const BluetoothOptions& opts) {
        return {
            PermissionType::Bluetooth,
            "bluetooth",
            true,
            "Bluetooth is required to connect to external hardware devices.",
            {},
            {"NSBluetoothAlwaysUsageDescription"},
            opts
        };
    }

    static std::optional<PermissionType> parse_type(std::string_view name) {
        if (name == "camera") return PermissionType::Camera;
        if (name == "microphone" || name == "audio" || name == "record_audio") return PermissionType::Microphone;
        if (name == "location") return PermissionType::Location;
        if (name == "notifications" || name == "notification") return PermissionType::Notifications;
        if (name == "storage") return PermissionType::Storage;
        if (name == "bluetooth") return PermissionType::Bluetooth;
        return std::nullopt;
    }

    static std::string type_to_string(PermissionType type) {
        return get_descriptor(type).key_name;
    }

    static std::vector<PermissionDescriptor> default_permissions() {
        return {
            get_descriptor(PermissionType::Camera),
            get_descriptor(PermissionType::Microphone),
            get_descriptor(PermissionType::Location),
            get_descriptor(PermissionType::Notifications)
        };
    }

    static std::vector<PermissionDescriptor> parse_yaml(std::string_view yaml_content) {
        std::vector<PermissionDescriptor> result;
        std::istringstream stream((std::string(yaml_content)));
        std::string line;
        bool in_permissions = false;
        std::optional<PermissionType> current_type;
        bool current_enabled = true;
        std::string current_reason;
        BluetoothOptions current_bt_opts;

        auto finalize_current = [&]() {
            if (current_type.has_value()) {
                auto desc = get_descriptor(*current_type);
                desc.enabled = current_enabled;
                if (!current_reason.empty()) {
                    desc.reason = current_reason;
                }
                if (*current_type == PermissionType::Bluetooth) {
                    desc.bluetooth_opts = current_bt_opts;
                }
                result.push_back(desc);
                current_type = std::nullopt;
                current_enabled = true;
                current_reason.clear();
                current_bt_opts = BluetoothOptions{};
            }
        };

        while (std::getline(stream, line)) {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(" \t\r\n");
            std::string trimmed = line.substr(first, last - first + 1);

            if (trimmed == "permissions:") {
                finalize_current();
                in_permissions = true;
                continue;
            }

            if (!in_permissions) continue;

            if (line[0] != ' ' && line[0] != '\t' && line.back() == ':') {
                finalize_current();
                in_permissions = false;
                continue;
            }

            if (line.rfind("  ", 0) == 0 && line.find("    ") != 0 && trimmed.back() == ':') {
                finalize_current();
                std::string key = trimmed.substr(0, trimmed.size() - 1);
                current_type = parse_type(key);
                continue;
            }

            if (current_type.has_value()) {
                if (trimmed.rfind("enabled:", 0) == 0) {
                    std::string val = trimmed.substr(8);
                    size_t vf = val.find_first_not_of(" \t\"'");
                    size_t vl = val.find_last_not_of(" \t\"'");
                    if (vf != std::string::npos && vl != std::string::npos) {
                        std::string clean = val.substr(vf, vl - vf + 1);
                        current_enabled = (clean == "true" || clean == "1" || clean == "yes");
                    }
                } else if (trimmed.rfind("reason:", 0) == 0) {
                    std::string val = trimmed.substr(7);
                    size_t vf = val.find_first_not_of(" \t\"'");
                    size_t vl = val.find_last_not_of(" \t\"'");
                    if (vf != std::string::npos && vl != std::string::npos) {
                        current_reason = val.substr(vf, vl - vf + 1);
                    }
                } else if (trimmed.rfind("scan:", 0) == 0) {
                    std::string val = trimmed.substr(5);
                    current_bt_opts.scan = (val.find("true") != std::string::npos);
                } else if (trimmed.rfind("connect:", 0) == 0) {
                    std::string val = trimmed.substr(8);
                    current_bt_opts.connect = (val.find("true") != std::string::npos);
                } else if (trimmed.rfind("advertise:", 0) == 0) {
                    std::string val = trimmed.substr(10);
                    current_bt_opts.advertise = (val.find("true") != std::string::npos);
                } else if (trimmed.rfind("never_for_location:", 0) == 0) {
                    std::string val = trimmed.substr(19);
                    current_bt_opts.never_for_location = (val.find("true") != std::string::npos);
                }
            }
        }
        finalize_current();
        return result;
    }

    static std::string serialize_yaml(const std::vector<PermissionDescriptor>& perms) {
        std::ostringstream out;
        out << "permissions:\n";
        for (const auto& p : perms) {
            out << "  " << p.key_name << ":\n"
                << "    enabled: " << (p.enabled ? "true" : "false") << "\n"
                << "    reason: \"" << p.reason << "\"\n";
            if (p.type == PermissionType::Bluetooth) {
                out << "    connect: " << (p.bluetooth_opts.connect ? "true" : "false") << "\n"
                    << "    scan: " << (p.bluetooth_opts.scan ? "true" : "false") << "\n"
                    << "    never_for_location: " << (p.bluetooth_opts.never_for_location ? "true" : "false") << "\n"
                    << "    advertise: " << (p.bluetooth_opts.advertise ? "true" : "false") << "\n";
            }
        }
        return out.str();
    }

    static bool inject_android_permissions(const std::filesystem::path& manifest_path, const std::vector<PermissionDescriptor>& perms) {
        if (!std::filesystem::exists(manifest_path)) return false;

        std::ifstream in(manifest_path);
        if (!in.is_open()) return false;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        bool modified = false;
        std::ostringstream to_inject;

        auto inject_entry = [&](const AndroidPermissionEntry& entry) {
            std::string tag = "android:name=\"" + entry.name + "\"";
            if (content.find(tag) == std::string::npos) {
                to_inject << "    <uses-permission android:name=\"" << entry.name << "\"";
                if (entry.max_api > 0) {
                    to_inject << " android:maxSdkVersion=\"" << entry.max_api << "\"";
                }
                if (!entry.flags.empty()) {
                    to_inject << " android:usesPermissionFlags=\"" << entry.flags << "\"";
                }
                to_inject << " />\n";
                modified = true;
            }
            if (!entry.feature_req.empty()) {
                std::string feat_tag = "android:name=\"" + entry.feature_req + "\"";
                if (content.find(feat_tag) == std::string::npos) {
                    to_inject << "    <uses-feature android:name=\"" << entry.feature_req << "\" android:required=\"false\" />\n";
                    modified = true;
                }
            }
        };

        for (const auto& p : perms) {
            if (!p.enabled) continue;

            if (p.type == PermissionType::Bluetooth) {
                // Granular Bluetooth permissions based on options
                if (p.bluetooth_opts.connect) {
                    inject_entry({"android.permission.BLUETOOTH_CONNECT", 31, 0, "", ""});
                    inject_entry({"android.permission.BLUETOOTH", 0, 30, "", ""});
                }
                if (p.bluetooth_opts.scan) {
                    std::string flags = p.bluetooth_opts.never_for_location ? "neverForLocation" : "";
                    inject_entry({"android.permission.BLUETOOTH_SCAN", 31, 0, flags, ""});
                    inject_entry({"android.permission.BLUETOOTH_ADMIN", 0, 30, "", ""});
                    if (!p.bluetooth_opts.never_for_location) {
                        inject_entry({"android.permission.ACCESS_FINE_LOCATION", 0, 0, "", ""});
                    }
                }
                if (p.bluetooth_opts.advertise) {
                    inject_entry({"android.permission.BLUETOOTH_ADVERTISE", 31, 0, "", ""});
                }
            } else {
                for (const auto& entry : p.android_entries) {
                    inject_entry(entry);
                }
            }
        }

        if (!modified) return true; // Already up to date

        size_t app_pos = content.find("<application");
        if (app_pos != std::string::npos) {
            content.insert(app_pos, to_inject.str());
        } else {
            size_t end_pos = content.find("</manifest>");
            if (end_pos != std::string::npos) {
                content.insert(end_pos, to_inject.str());
            } else {
                content += "\n" + to_inject.str();
            }
        }

        std::ofstream out(manifest_path);
        if (!out.is_open()) return false;
        out << content;
        return true;
    }

    static bool inject_ios_plist_permissions(const std::filesystem::path& plist_path, const std::vector<PermissionDescriptor>& perms) {
        if (!std::filesystem::exists(plist_path)) return false;

        std::ifstream in(plist_path);
        if (!in.is_open()) return false;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        bool modified = false;
        std::ostringstream to_inject;

        for (const auto& p : perms) {
            if (!p.enabled) continue;
            for (const auto& key : p.ios_plist_keys) {
                std::string key_tag = "<key>" + key + "</key>";
                if (content.find(key_tag) == std::string::npos) {
                    to_inject << "\t<key>" << key << "</key>\n"
                              << "\t<string>" << p.reason << "</string>\n";
                    modified = true;
                }
            }
        }

        if (!modified) return true;

        size_t last_dict = content.rfind("</dict>");
        if (last_dict != std::string::npos) {
            content.insert(last_dict, to_inject.str());
        } else {
            content += "\n" + to_inject.str();
        }

        std::ofstream out(plist_path);
        if (!out.is_open()) return false;
        out << content;
        return true;
    }

    static bool add_permission_to_yaml(const std::filesystem::path& yaml_path, PermissionType type, const std::string& custom_reason = "") {
        auto desc = get_descriptor(type);
        if (!custom_reason.empty()) {
            desc.reason = custom_reason;
        }
        desc.enabled = true;

        std::vector<PermissionDescriptor> existing;
        if (std::filesystem::exists(yaml_path)) {
            std::ifstream in(yaml_path);
            if (in.is_open()) {
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                existing = parse_yaml(content);
            }
        }

        bool updated = false;
        for (auto& p : existing) {
            if (p.type == type) {
                p.enabled = true;
                if (!custom_reason.empty()) p.reason = custom_reason;
                updated = true;
                break;
            }
        }
        if (!updated) {
            existing.push_back(desc);
        }

        std::string file_content;
        if (std::filesystem::exists(yaml_path)) {
            std::ifstream in(yaml_path);
            if (in.is_open()) {
                file_content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            }
        }

        size_t perm_pos = file_content.find("permissions:");
        if (perm_pos != std::string::npos) {
            size_t next_sec = file_content.find("\n\n", perm_pos);
            if (next_sec == std::string::npos) {
                file_content = file_content.substr(0, perm_pos);
            } else {
                file_content.erase(perm_pos, next_sec - perm_pos + 2);
            }
        }

        if (!file_content.empty() && file_content.back() != '\n') {
            file_content += "\n";
        }
        file_content += "\n" + serialize_yaml(existing);

        std::ofstream out(yaml_path);
        if (!out.is_open()) return false;
        out << file_content;
        return true;
    }

    static bool sync_project_permissions(const std::filesystem::path& project_root) {
        auto yaml_path = project_root / "pingcore.yaml";
        if (!std::filesystem::exists(yaml_path)) {
            return false;
        }

        std::ifstream in(yaml_path);
        if (!in.is_open()) return false;
        std::string yaml_content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        auto perms = parse_yaml(yaml_content);
        if (perms.empty()) {
            perms = default_permissions();
        }

        auto android_manifest = project_root / "platforms" / "android" / "app" / "src" / "main" / "AndroidManifest.xml";
        if (std::filesystem::exists(android_manifest)) {
            inject_android_permissions(android_manifest, perms);
        }

        auto ios_plist = project_root / "platforms" / "ios" / "MyApp" / "Info.plist";
        if (std::filesystem::exists(ios_plist)) {
            inject_ios_plist_permissions(ios_plist, perms);
        }

        return true;
    }
};

class PermissionRegistry {
public:
    static PermissionRegistry& instance() {
        static PermissionRegistry s_inst;
        return s_inst;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_.clear();
    }

    void declare_permission(const std::string& name, const std::string& reason) {
        std::lock_guard<std::mutex> lock(mutex_);
        permissions_[name] = reason;
    }

    bool has_permission(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return permissions_.find(name) != permissions_.end();
    }

    std::string get_reason(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = permissions_.find(name);
        return it != permissions_.end() ? it->second : "";
    }

    std::vector<std::string> android_manifest_permissions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> perms;
        for (const auto& [name, _] : permissions_) {
            if (name == "camera") perms.push_back("android.permission.CAMERA");
            else if (name == "location") perms.push_back("android.permission.ACCESS_FINE_LOCATION");
            else if (name == "microphone") perms.push_back("android.permission.RECORD_AUDIO");
            else if (name == "notifications") perms.push_back("android.permission.POST_NOTIFICATIONS");
            else if (name == "storage") perms.push_back("android.permission.READ_EXTERNAL_STORAGE");
            else if (name == "bluetooth") perms.push_back("android.permission.BLUETOOTH_CONNECT");
            else perms.push_back("android.permission." + name);
        }
        return perms;
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::string> permissions_;
};

} // namespace pingcore
