#include <pingdart/cli/packager.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <set>
#include <array>
#include <cstdio>

namespace pingdart::cli {

namespace {

bool write_text_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

uint64_t calculate_directory_size(const std::filesystem::path& dir) {
    uint64_t total = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) {
            total += entry.file_size(ec);
        }
    }
    return total;
}

} // namespace

std::string Packager::generate_macos_info_plist(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    ss << "<plist version=\"1.0\">\n";
    ss << "<dict>\n";
    ss << "    <key>CFBundlePackageType</key>\n";
    ss << "    <string>APPL</string>\n";
    ss << "    <key>CFBundleExecutable</key>\n";
    ss << "    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundleIdentifier</key>\n";
    ss << "    <string>" << descriptor.bundle_id << "</string>\n";
    ss << "    <key>CFBundleName</key>\n";
    ss << "    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundleDisplayName</key>\n";
    ss << "    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundleVersion</key>\n";
    ss << "    <string>" << descriptor.version << "</string>\n";
    ss << "    <key>CFBundleShortVersionString</key>\n";
    ss << "    <string>" << descriptor.short_version << "</string>\n";
    ss << "    <key>LSMinimumSystemVersion</key>\n";
    ss << "    <string>12.0</string>\n";
    ss << "    <key>NSHighResolutionCapable</key>\n";
    ss << "    <" << (descriptor.high_dpi_capable ? "true" : "false") << "/>\n";
    ss << "    <key>NSSupportsAutomaticGraphicsSwitching</key>\n";
    ss << "    <true/>\n";
    ss << "    <key>NSPrincipalClass</key>\n";
    ss << "    <string>NSApplication</string>\n";
    ss << "</dict>\n";
    ss << "</plist>\n";
    return ss.str();
}

std::string Packager::generate_windows_manifest(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    ss << "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n";
    ss << "  <assemblyIdentity version=\"" << descriptor.version << ".0\" processorArchitecture=\"*\" name=\""
       << descriptor.bundle_id << "\" type=\"win32\"/>\n";
    ss << "  <description>" << descriptor.app_name << " Native Application</description>\n";
    ss << "  <dependency>\n";
    ss << "    <dependentAssembly>\n";
    ss << "      <assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>\n";
    ss << "    </dependentAssembly>\n";
    ss << "  </dependency>\n";
    ss << "  <application xmlns=\"urn:schemas-microsoft-com:asm.v3\">\n";
    ss << "    <windowsSettings>\n";
    ss << "      <dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">true/pm</dpiAware>\n";
    ss << "      <dpiAwareness xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">PerMonitorV2, PerMonitor</dpiAwareness>\n";
    ss << "      <longPathAware xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">true</longPathAware>\n";
    ss << "    </windowsSettings>\n";
    ss << "  </application>\n";
    ss << "  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">\n";
    ss << "    <application>\n";
    ss << "      <!-- Windows 10 & 11 -->\n";
    ss << "      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>\n";
    ss << "    </application>\n";
    ss << "  </compatibility>\n";
    ss << "</assembly>\n";
    return ss.str();
}

std::string Packager::generate_linux_apprun(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "#!/bin/sh\n";
    ss << "# PingDart Linux AppRun Launcher\n";
    ss << "HERE=\"$(dirname \"$(readlink -f \"${0}\")\")\"\n";
    ss << "export LD_LIBRARY_PATH=\"${HERE}/usr/lib:${LD_LIBRARY_PATH}\"\n";
    ss << "export XDG_DATA_DIRS=\"${HERE}/usr/share:${XDG_DATA_DIRS}\"\n";
    ss << "exec \"${HERE}/usr/bin/" << descriptor.app_name << "\" \"$@\"\n";
    return ss.str();
}

std::string Packager::generate_linux_desktop_entry(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "[Desktop Entry]\n";
    ss << "Type=Application\n";
    ss << "Name=" << descriptor.app_name << "\n";
    ss << "Exec=" << descriptor.app_name << "\n";
    ss << "Icon=" << descriptor.app_name << "\n";
    ss << "Categories=" << descriptor.category << ";\n";
    ss << "Terminal=false\n";
    ss << "StartupNotify=true\n";
    return ss.str();
}

core::Result<PackageResult> Packager::package_macos(const PackageDescriptor& descriptor) {
    std::error_code ec;
    std::filesystem::path out_root = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + ".app");
    std::filesystem::path contents_dir = out_root / "Contents";
    std::filesystem::path macos_dir = contents_dir / "MacOS";
    std::filesystem::path res_dir = contents_dir / "Resources";
    std::filesystem::path fw_dir = contents_dir / "Frameworks";

    std::filesystem::create_directories(macos_dir, ec);
    std::filesystem::create_directories(res_dir, ec);
    std::filesystem::create_directories(fw_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create macOS bundle directories: " + ec.message());
    }

    // 1. Write Info.plist
    std::string plist_content = generate_macos_info_plist(descriptor);
    std::filesystem::path plist_path = contents_dir / "Info.plist";
    if (!write_text_file(plist_path, plist_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write Info.plist");
    }

    // 2. Write PkgInfo
    std::filesystem::path pkginfo_path = contents_dir / "PkgInfo";
    write_text_file(pkginfo_path, "APPL\\?\\?\\?\\?");

    // 3. Copy executable
    std::filesystem::path target_exec = macos_dir / descriptor.app_name;
    if (!descriptor.executable_path.empty() && std::filesystem::exists(descriptor.executable_path, ec)) {
        std::filesystem::copy_file(descriptor.executable_path, target_exec,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return core::Status(core::StatusCode::Internal, "Failed to copy executable into bundle: " + ec.message());
        }
        // Ensure executable permissions (rwxr-xr-x)
        std::filesystem::permissions(target_exec,
                                     std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                                     std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::replace, ec);
    }

    // 4. Bundle Fonts (strictly enforcing redistribution licensing)
    FontBundler font_bundler;
    std::vector<BundledFontInfo> approved_fonts;
    for (const auto& font_path : descriptor.font_paths) {
        auto eval_res = font_bundler.evaluate_font_for_bundle(font_path);
        if (eval_res.has_value()) {
            approved_fonts.push_back(std::move(eval_res).value());
        }
    }
    if (!approved_fonts.empty()) {
        (void)font_bundler.copy_fonts_to_bundle(approved_fonts, res_dir.string());
    }

    // 5. Automated Dynamic Dependency Bundling & Relocatable Framework Isolation (ENG-602)
#if defined(__APPLE__)
    if (std::filesystem::exists(target_exec, ec)) {
        // Configure main executable rpath pointing to bundled Frameworks
        std::string add_rpath = "install_name_tool -add_rpath \"@executable_path/../Frameworks\" \"" + target_exec.string() + "\" 2>/dev/null";
        (void)std::system(add_rpath.c_str());

        std::vector<std::string> to_process = {target_exec.string()};
        std::set<std::string> processed;

        auto find_dylib_source = [](const std::string& filename, const std::string& parent_hint) -> std::filesystem::path {
            std::error_code sec;
            if (!parent_hint.empty()) {
                std::filesystem::path hint_dir = std::filesystem::path(parent_hint).parent_path();
                if (std::filesystem::exists(hint_dir / filename, sec)) return hint_dir / filename;
                if (std::filesystem::exists(hint_dir.parent_path() / "lib" / filename, sec)) return hint_dir.parent_path() / "lib" / filename;
            }
            std::filesystem::path hb_lib = "/opt/homebrew/lib";
            if (std::filesystem::exists(hb_lib / filename, sec)) return hb_lib / filename;
            std::filesystem::path usr_loc = "/usr/local/lib";
            if (std::filesystem::exists(usr_loc / filename, sec)) return usr_loc / filename;

            // Search /opt/homebrew/opt/*/lib
            std::filesystem::path hb_opt = "/opt/homebrew/opt";
            if (std::filesystem::exists(hb_opt, sec)) {
                for (const auto& pkg_entry : std::filesystem::directory_iterator(hb_opt, sec)) {
                    if (pkg_entry.is_directory()) {
                        std::filesystem::path cand = pkg_entry.path() / "lib" / filename;
                        if (std::filesystem::exists(cand, sec)) return cand;
                    }
                }
            }
            return {};
        };

        while (!to_process.empty()) {
            std::string curr = to_process.back();
            to_process.pop_back();
            if (processed.count(curr)) continue;
            processed.insert(curr);

            std::string otool_cmd = "otool -L \"" + curr + "\" 2>/dev/null";
            FILE* pipe = popen(otool_cmd.c_str(), "r");
            if (!pipe) continue;

            std::array<char, 256> obuf{};
            std::string otool_out;
            while (fgets(obuf.data(), static_cast<int>(obuf.size()), pipe) != nullptr) {
                otool_out += obuf.data();
            }
            pclose(pipe);

            std::istringstream stream(otool_out);
            std::string line;
            while (std::getline(stream, line)) {
                size_t start = line.find_first_not_of(" \t");
                if (start == std::string::npos) continue;
                size_t end = line.find(' ', start);
                if (end == std::string::npos) continue;
                std::string dylib_path = line.substr(start, end - start);

                // Skip standard Apple system libraries
                if (dylib_path.rfind("/usr/lib/", 0) == 0 ||
                    dylib_path.rfind("/System/Library/", 0) == 0) {
                    continue;
                }

                std::string dylib_filename;
                std::filesystem::path source_path;

                if (dylib_path.rfind("@rpath/", 0) == 0) {
                    dylib_filename = dylib_path.substr(7);
                    std::filesystem::path dest_p = fw_dir / dylib_filename;
                    if (!std::filesystem::exists(dest_p, ec)) {
                        source_path = find_dylib_source(dylib_filename, curr);
                    }
                } else if (dylib_path.rfind("@loader_path/", 0) == 0) {
                    dylib_filename = dylib_path.substr(13);
                    std::filesystem::path dest_p = fw_dir / dylib_filename;
                    if (!std::filesystem::exists(dest_p, ec)) {
                        source_path = find_dylib_source(dylib_filename, curr);
                    }
                } else if (dylib_path.rfind("@executable_path", 0) == 0) {
                    // Already packaged reference
                    continue;
                } else {
                    // Absolute path to library
                    std::filesystem::path p(dylib_path);
                    dylib_filename = p.filename().string();
                    source_path = p;
                }

                std::filesystem::path dest_p = fw_dir / dylib_filename;

                if (!source_path.empty() && std::filesystem::exists(source_path, ec) && !std::filesystem::exists(dest_p, ec)) {
                    std::filesystem::copy_file(source_path, dest_p, std::filesystem::copy_options::overwrite_existing, ec);
                    std::filesystem::permissions(dest_p,
                                                 std::filesystem::perms::all,
                                                 std::filesystem::perm_options::replace, ec);
                    std::string id_cmd = "install_name_tool -id \"@rpath/" + dylib_filename + "\" \"" + dest_p.string() + "\" 2>/dev/null";
                    (void)std::system(id_cmd.c_str());
                    to_process.push_back(dest_p.string());
                }

                // Rewrite load reference in the referencing binary to @executable_path/../Frameworks/
                std::string change_cmd = "install_name_tool -change \"" + dylib_path + "\" \"@executable_path/../Frameworks/" + dylib_filename + "\" \"" + curr + "\" 2>/dev/null";
                (void)std::system(change_cmd.c_str());
            }
        }

        // Ad-hoc or Identity Code Signing in proper inside-out dependency order (ENG-602)
        if (descriptor.codesign_bundle) {
            std::string identity = descriptor.signing_identity.empty() ? "-" : descriptor.signing_identity;
            std::string runtime_opt = (identity != "-") ? " --options runtime" : "";

            // 1. Sign nested dynamic libraries in Contents/Frameworks/ first
            for (const auto& fw_entry : std::filesystem::directory_iterator(fw_dir, ec)) {
                if (fw_entry.path().extension() == ".dylib" || fw_entry.path().string().find(".dylib") != std::string::npos) {
                    std::string fw_sign = "codesign -f -s \"" + identity + "\" --preserve-metadata=identifier,entitlements,flags \"" + fw_entry.path().string() + "\" 2>/dev/null";
                    (void)std::system(fw_sign.c_str());
                }
            }

            // 2. Sign the main executable
            std::string exec_sign = "codesign -f -s \"" + identity + "\"" + runtime_opt + " \"" + target_exec.string() + "\" 2>/dev/null";
            (void)std::system(exec_sign.c_str());

            // 3. Sign the top-level bundle
            std::string sign_cmd = "codesign -f -s \"" + identity + "\"" + runtime_opt + " \"" + out_root.string() + "\" 2>/dev/null";
            (void)std::system(sign_cmd.c_str());
        }
    }
#endif

    // 6. Gather Result
    PackageResult result;
    result.success = true;
    result.format = PackageFormat::MacOSAppBundle;
    result.package_root_path = out_root.string();
    result.executable_bundled_path = target_exec.string();
    result.manifest_or_plist_path = plist_path.string();
    result.total_size_bytes = calculate_directory_size(out_root);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_root, ec)) {
        if (entry.is_regular_file(ec)) {
            result.bundled_files.push_back(entry.path().string());
        }
    }

    return result;
}

core::Result<PackageResult> Packager::package_windows(const PackageDescriptor& descriptor) {
    std::error_code ec;
    std::filesystem::path out_root = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + "_windows_x64");
    std::filesystem::path res_dir = out_root / "resources";

    std::filesystem::create_directories(out_root, ec);
    std::filesystem::create_directories(res_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create Windows package directory: " + ec.message());
    }

    // 1. Copy executable
    std::filesystem::path target_exec = out_root / (descriptor.app_name + ".exe");
    if (!descriptor.executable_path.empty() && std::filesystem::exists(descriptor.executable_path, ec)) {
        std::filesystem::copy_file(descriptor.executable_path, target_exec,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return core::Status(core::StatusCode::Internal, "Failed to copy executable: " + ec.message());
        }
    }

    // 2. Write Manifest
    std::string manifest_content = generate_windows_manifest(descriptor);
    std::filesystem::path manifest_path = out_root / (descriptor.app_name + ".exe.manifest");
    if (!write_text_file(manifest_path, manifest_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write application manifest");
    }

    // 3. Bundle Fonts
    FontBundler font_bundler;
    std::vector<BundledFontInfo> approved_fonts;
    for (const auto& font_path : descriptor.font_paths) {
        auto eval_res = font_bundler.evaluate_font_for_bundle(font_path);
        if (eval_res.has_value()) {
            approved_fonts.push_back(std::move(eval_res).value());
        }
    }
    if (!approved_fonts.empty()) {
        (void)font_bundler.copy_fonts_to_bundle(approved_fonts, res_dir.string());
    }

    PackageResult result;
    result.success = true;
    result.format = PackageFormat::WindowsExecutablePackage;
    result.package_root_path = out_root.string();
    result.executable_bundled_path = target_exec.string();
    result.manifest_or_plist_path = manifest_path.string();
    result.total_size_bytes = calculate_directory_size(out_root);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_root, ec)) {
        if (entry.is_regular_file(ec)) {
            result.bundled_files.push_back(entry.path().string());
        }
    }

    return result;
}

core::Result<PackageResult> Packager::package_linux(const PackageDescriptor& descriptor) {
    std::error_code ec;
    std::filesystem::path out_root = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + ".AppDir");
    std::filesystem::path bin_dir = out_root / "usr" / "bin";
    std::filesystem::path lib_dir = out_root / "usr" / "lib";
    std::filesystem::path share_dir = out_root / "usr" / "share" / descriptor.app_name;

    std::filesystem::create_directories(bin_dir, ec);
    std::filesystem::create_directories(lib_dir, ec);
    std::filesystem::create_directories(share_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create Linux AppDir structure: " + ec.message());
    }

    // 1. AppRun Launcher
    std::string apprun_content = generate_linux_apprun(descriptor);
    std::filesystem::path apprun_path = out_root / "AppRun";
    if (!write_text_file(apprun_path, apprun_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write AppRun");
    }
    std::filesystem::permissions(apprun_path,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                                 std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                                 std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
                                 std::filesystem::perm_options::replace, ec);

    // 2. Desktop Entry
    std::string desktop_content = generate_linux_desktop_entry(descriptor);
    std::filesystem::path desktop_path = out_root / (descriptor.app_name + ".desktop");
    if (!write_text_file(desktop_path, desktop_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write .desktop file");
    }

    // 3. Copy executable
    std::filesystem::path target_exec = bin_dir / descriptor.app_name;
    if (!descriptor.executable_path.empty() && std::filesystem::exists(descriptor.executable_path, ec)) {
        std::filesystem::copy_file(descriptor.executable_path, target_exec,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return core::Status(core::StatusCode::Internal, "Failed to copy executable: " + ec.message());
        }
        std::filesystem::permissions(target_exec,
                                     std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                                     std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::replace, ec);
    }

    // 4. Linux AppImage Generation (ENG-602: if requested and appimagetool present)
    if (descriptor.create_appimage) {
        std::filesystem::path appimage_out = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + "-x86_64.AppImage");
        std::string test_tool = "which appimagetool >/dev/null 2>&1";
        if (std::system(test_tool.c_str()) == 0) {
            std::string build_cmd = "appimagetool \"" + out_root.string() + "\" \"" + appimage_out.string() + "\" >/dev/null 2>&1";
            (void)std::system(build_cmd.c_str());
        }
    }

    PackageResult result;
    result.success = true;
    result.format = PackageFormat::LinuxAppDir;
    result.package_root_path = out_root.string();
    result.executable_bundled_path = target_exec.string();
    result.manifest_or_plist_path = desktop_path.string();
    result.total_size_bytes = calculate_directory_size(out_root);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_root, ec)) {
        if (entry.is_regular_file(ec)) {
            result.bundled_files.push_back(entry.path().string());
        }
    }

    return result;
}

std::string Packager::generate_android_manifest(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    ss << "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n";
    ss << "    package=\"" << descriptor.bundle_id << "\">\n";
    ss << "\n";
    ss << "    <uses-feature android:name=\"android.hardware.vulkan.version\" android:version=\"0x401000\" android:required=\"true\" />\n";
    ss << "    <uses-feature android:name=\"android.hardware.touchscreen\" android:required=\"true\" />\n";
    ss << "\n";
    ss << "    <application\n";
    ss << "        android:label=\"" << descriptor.app_name << "\"\n";
    ss << "        android:hasCode=\"false\"\n";
    ss << "        android:extractNativeLibs=\"true\">\n";
    ss << "        <activity\n";
    ss << "            android:name=\"android.app.NativeActivity\"\n";
    ss << "            android:label=\"" << descriptor.app_name << "\"\n";
    ss << "            android:configChanges=\"orientation|keyboardHidden|screenSize|screenLayout\"\n";
    ss << "            android:exported=\"true\">\n";
    ss << "            <meta-data android:name=\"android.app.lib_name\" android:value=\"" << descriptor.app_name << "\" />\n";
    ss << "            <intent-filter>\n";
    ss << "                <action android:name=\"android.intent.action.MAIN\" />\n";
    ss << "                <category android:name=\"android.intent.category.LAUNCHER\" />\n";
    ss << "            </intent-filter>\n";
    ss << "        </activity>\n";
    ss << "    </application>\n";
    ss << "</manifest>\n";
    return ss.str();
}

std::string Packager::generate_android_build_gradle(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "plugins {\n";
    ss << "    id(\"com.android.application\")\n";
    ss << "}\n\n";
    ss << "android {\n";
    ss << "    namespace = \"" << descriptor.bundle_id << "\"\n";
    ss << "    compileSdk = 34\n\n";
    ss << "    defaultConfig {\n";
    ss << "        applicationId = \"" << descriptor.bundle_id << "\"\n";
    ss << "        minSdk = 26\n";
    ss << "        targetSdk = 34\n";
    ss << "        versionCode = 1\n";
    ss << "        versionName = \"" << descriptor.version << "\"\n\n";
    ss << "        ndk {\n";
    ss << "            abiFilters.addAll(listOf(\"arm64-v8a\"))\n";
    ss << "        }\n\n";
    ss << "        externalNativeBuild {\n";
    ss << "            cmake {\n";
    ss << "                cppFlags.addAll(listOf(\"-std=c++20\", \"-fexceptions\", \"-frtti\"))\n";
    ss << "                arguments.addAll(listOf(\"-DANDROID_STL=c++_shared\"))\n";
    ss << "            }\n";
    ss << "        }\n";
    ss << "    }\n\n";
    ss << "    buildTypes {\n";
    ss << "        release {\n";
    ss << "            isMinifyEnabled = false\n";
    ss << "        }\n";
    ss << "    }\n\n";
    ss << "    externalNativeBuild {\n";
    ss << "        cmake {\n";
    ss << "            path = file(\"CMakeLists.txt\")\n";
    ss << "            version = \"3.26.0+\"\n";
    ss << "        }\n";
    ss << "    }\n";
    ss << "}\n";
    return ss.str();
}

std::string Packager::generate_android_native_main(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "// " << descriptor.app_name << " Android NDK NativeActivity Entry Point (MOBILE-102)\n";
    ss << "#include <pingdart/core/runtime.h>\n";
    ss << "#include <pingdart/pal/android/android_platform.h>\n";
    ss << "#include <android/log.h>\n\n";
    ss << "#define LOG_TAG \"" << descriptor.app_name << "\"\n";
    ss << "#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)\n\n";
    ss << "struct android_app;\n\n";
    ss << "extern \"C\" void android_main(struct android_app* state) {\n";
    ss << "    LOGI(\"Booting PingDart Android Native App: " << descriptor.app_name << "\");\n";
    ss << "    (void)pingdart::core::Runtime::initialize();\n";
    ss << "    pingdart::pal::android::AndroidPlatformHost host;\n";
    ss << "    (void)host.initialize();\n";
    ss << "    (void)host.create_window(\"" << descriptor.app_name << "\", {1080, 2400});\n";
    ss << "    while (!host.is_quit_requested()) {\n";
    ss << "        host.event_pump().poll_events();\n";
    ss << "    }\n";
    ss << "    host.shutdown();\n";
    ss << "    pingdart::core::Runtime::shutdown();\n";
    ss << "}\n";
    return ss.str();
}

std::string Packager::generate_ios_info_plist(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    ss << "<plist version=\"1.0\">\n<dict>\n";
    ss << "    <key>CFBundleDevelopmentRegion</key>\n    <string>en</string>\n";
    ss << "    <key>CFBundleDisplayName</key>\n    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundleExecutable</key>\n    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundleIdentifier</key>\n    <string>" << descriptor.bundle_id << "</string>\n";
    ss << "    <key>CFBundleInfoDictionaryVersion</key>\n    <string>6.0</string>\n";
    ss << "    <key>CFBundleName</key>\n    <string>" << descriptor.app_name << "</string>\n";
    ss << "    <key>CFBundlePackageType</key>\n    <string>APPL</string>\n";
    ss << "    <key>CFBundleShortVersionString</key>\n    <string>" << descriptor.short_version << "</string>\n";
    ss << "    <key>CFBundleVersion</key>\n    <string>" << descriptor.version << "</string>\n";
    ss << "    <key>LSRequiresIPhoneOS</key>\n    <true/>\n";
    ss << "    <key>UIDeviceFamily</key>\n    <array>\n        <integer>1</integer>\n        <integer>2</integer>\n    </array>\n";
    ss << "    <key>UIRequiredDeviceCapabilities</key>\n    <array>\n        <string>arm64</string>\n        <string>metal</string>\n    </array>\n";
    ss << "    <key>CADisableMinimumFrameDurationOnPhone</key>\n    <true/>\n";
    ss << "</dict>\n</plist>\n";
    return ss.str();
}

std::string Packager::generate_ios_view_controller_mm(const PackageDescriptor& descriptor) {
    std::ostringstream ss;
    ss << "// " << descriptor.app_name << " iOS View Controller (MOBILE-103)\n";
    ss << "#import <UIKit/UIKit.h>\n";
    ss << "#import <Metal/Metal.h>\n";
    ss << "#import <QuartzCore/CAMetalLayer.h>\n";
    ss << "#include <pingdart/pal/ios/ios_platform.h>\n\n";
    ss << "@interface PingDartViewController : UIViewController\n@end\n\n";
    ss << "@implementation PingDartViewController {\n";
    ss << "    CAMetalLayer* _metalLayer;\n";
    ss << "    std::unique_ptr<pingdart::pal::ios::IOSPlatformHost> _platformHost;\n";
    ss << "    std::unique_ptr<pingdart::pal::ios::IOSWindow> _window;\n";
    ss << "}\n\n";
    ss << "- (void)viewDidLoad {\n";
    ss << "    [super viewDidLoad];\n";
    ss << "    self.view.backgroundColor = [UIColor blackColor];\n";
    ss << "    _metalLayer = [CAMetalLayer layer];\n";
    ss << "    _metalLayer.device = MTLCreateSystemDefaultDevice();\n";
    ss << "    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;\n";
    ss << "    _metalLayer.frame = self.view.bounds;\n";
    ss << "    [self.view.layer addSublayer:_metalLayer];\n\n";
    ss << "    _platformHost = std::make_unique<pingdart::pal::ios::IOSPlatformHost>();\n";
    ss << "    (void)_platformHost->initialize();\n";
    ss << "    auto win_res = _platformHost->create_window(\"" << descriptor.app_name << "\",\n";
    ss << "        {static_cast<uint32_t>(self.view.bounds.size.width),\n";
    ss << "         static_cast<uint32_t>(self.view.bounds.size.height)});\n";
    ss << "    if (win_res.has_value()) {\n";
    ss << "        _window.reset(static_cast<pingdart::pal::ios::IOSWindow*>(win_res.value().release()));\n";
    ss << "        _window->setup_metal_surface(\n";
    ss << "            (__bridge void*)self.view,\n";
    ss << "            (__bridge void*)_metalLayer,\n";
    ss << "            static_cast<uint32_t>(self.view.bounds.size.width),\n";
    ss << "            static_cast<uint32_t>(self.view.bounds.size.height),\n";
    ss << "            [UIScreen mainScreen].scale\n";
    ss << "        );\n";
    ss << "    }\n";
    ss << "}\n\n";
    ss << "- (void)viewSafeAreaInsetsDidChange {\n";
    ss << "    [super viewSafeAreaInsetsDidChange];\n";
    ss << "    if (_window) {\n";
    ss << "        UIEdgeInsets insets = self.view.safeAreaInsets;\n";
    ss << "        pingdart::pal::ios::IOSSafeAreaInsets pd_insets{\n";
    ss << "            static_cast<float>(insets.top),\n";
    ss << "            static_cast<float>(insets.bottom),\n";
    ss << "            static_cast<float>(insets.left),\n";
    ss << "            static_cast<float>(insets.right)\n";
    ss << "        };\n";
    ss << "        _window->update_safe_area_insets(pd_insets);\n";
    ss << "    }\n";
    ss << "}\n\n";
    ss << "@end\n";
    return ss.str();
}

core::Result<PackageResult> Packager::package_android(const PackageDescriptor& descriptor) {
    std::error_code ec;
    std::filesystem::path out_root = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + "_Android");
    std::filesystem::path app_dir = out_root / "app";
    std::filesystem::path src_dir = app_dir / "src" / "main";
    std::filesystem::path cpp_dir = src_dir / "cpp";
    std::filesystem::path assets_dir = src_dir / "assets" / "fonts";

    std::filesystem::create_directories(cpp_dir, ec);
    std::filesystem::create_directories(assets_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create Android project directories: " + ec.message());
    }

    // 1. AndroidManifest.xml
    std::string manifest_content = generate_android_manifest(descriptor);
    std::filesystem::path manifest_path = src_dir / "AndroidManifest.xml";
    if (!write_text_file(manifest_path, manifest_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write AndroidManifest.xml");
    }

    // 2. build.gradle.kts
    std::string gradle_content = generate_android_build_gradle(descriptor);
    std::filesystem::path gradle_path = app_dir / "build.gradle.kts";
    write_text_file(gradle_path, gradle_content);

    // 3. native_activity_main.cpp
    std::string native_main = generate_android_native_main(descriptor);
    std::filesystem::path native_path = cpp_dir / "native_activity_main.cpp";
    write_text_file(native_path, native_main);

    // 4. Bundle approved fonts
    FontBundler font_bundler;
    std::vector<BundledFontInfo> approved_fonts;
    for (const auto& font_path : descriptor.font_paths) {
        auto eval_res = font_bundler.evaluate_font_for_bundle(font_path);
        if (eval_res.has_value()) {
            approved_fonts.push_back(std::move(eval_res).value());
        }
    }
    if (!approved_fonts.empty()) {
        (void)font_bundler.copy_fonts_to_bundle(approved_fonts, assets_dir.string());
    }

    PackageResult result;
    result.success = true;
    result.format = PackageFormat::AndroidProject;
    result.package_root_path = out_root.string();
    result.executable_bundled_path = native_path.string();
    result.manifest_or_plist_path = manifest_path.string();
    result.total_size_bytes = calculate_directory_size(out_root);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_root, ec)) {
        if (entry.is_regular_file(ec)) {
            result.bundled_files.push_back(entry.path().string());
        }
    }
    return result;
}

core::Result<PackageResult> Packager::package_ios(const PackageDescriptor& descriptor) {
    std::error_code ec;
    std::filesystem::path out_root = std::filesystem::path(descriptor.output_directory) / (descriptor.app_name + "_iOS");
    std::filesystem::path app_dir = out_root / descriptor.app_name;
    std::filesystem::path res_dir = app_dir / "Resources" / "fonts";

    std::filesystem::create_directories(app_dir, ec);
    std::filesystem::create_directories(res_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::Internal, "Failed to create iOS project directories: " + ec.message());
    }

    // 1. Info.plist
    std::string plist_content = generate_ios_info_plist(descriptor);
    std::filesystem::path plist_path = app_dir / "Info.plist";
    if (!write_text_file(plist_path, plist_content)) {
        return core::Status(core::StatusCode::Internal, "Failed to write iOS Info.plist");
    }

    // 2. PingDartViewController.mm
    std::string vc_content = generate_ios_view_controller_mm(descriptor);
    std::filesystem::path vc_path = app_dir / "PingDartViewController.mm";
    write_text_file(vc_path, vc_content);

    // 3. Bundle approved fonts
    FontBundler font_bundler;
    std::vector<BundledFontInfo> approved_fonts;
    for (const auto& font_path : descriptor.font_paths) {
        auto eval_res = font_bundler.evaluate_font_for_bundle(font_path);
        if (eval_res.has_value()) {
            approved_fonts.push_back(std::move(eval_res).value());
        }
    }
    if (!approved_fonts.empty()) {
        (void)font_bundler.copy_fonts_to_bundle(approved_fonts, res_dir.string());
    }

    PackageResult result;
    result.success = true;
    result.format = PackageFormat::IOSXcodeBundle;
    result.package_root_path = out_root.string();
    result.executable_bundled_path = vc_path.string();
    result.manifest_or_plist_path = plist_path.string();
    result.total_size_bytes = calculate_directory_size(out_root);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(out_root, ec)) {
        if (entry.is_regular_file(ec)) {
            result.bundled_files.push_back(entry.path().string());
        }
    }
    return result;
}

core::Result<PackageResult> Packager::create_package(const PackageDescriptor& descriptor) {
    TargetPlatform target = descriptor.target;
    if (target == TargetPlatform::Host) {
#if defined(__APPLE__)
        target = TargetPlatform::MacOSArm64;
#elif defined(_WIN32)
        target = TargetPlatform::WindowsX64;
#else
        target = TargetPlatform::LinuxX64;
#endif
    }

    switch (target) {
        case TargetPlatform::MacOSArm64:
        case TargetPlatform::MacOSX86_64:
            return package_macos(descriptor);
        case TargetPlatform::WindowsX64:
            return package_windows(descriptor);
        case TargetPlatform::LinuxX64:
            return package_linux(descriptor);
        case TargetPlatform::AndroidArm64:
            return package_android(descriptor);
        case TargetPlatform::IOSArm64:
            return package_ios(descriptor);
        case TargetPlatform::Host:
            return package_macos(descriptor);
    }
    return core::Status(core::StatusCode::InvalidArgument, "Unsupported package target");
}

} // namespace pingdart::cli
