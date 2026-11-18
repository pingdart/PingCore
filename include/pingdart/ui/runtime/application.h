// include/pingdart/ui/runtime/application.h
// PingDartUI Renderer-Independent App Entry Point & Lifecycle Controller (PKG-138)
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/runtime/reconciler.h>
#include <pingdart/ui/runtime/scheduler.h>
#include <pingdart/ui/theme.h>
#include <pingdart/ui/css_parser.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>

namespace pingdart::ui {

class App {
private:
    int argc_{0};
    char** argv_{nullptr};
    std::string title_{"PingDart Application"};
    std::string stylesheet_path_{""};
    std::string theme_name_{"system"};
    std::function<Component()> root_component_factory_{nullptr};
    Component root_vnode_{};

    std::shared_ptr<ComponentInstance> root_instance_{nullptr};
    bool is_running_{false};

public:
    App() = default;
    App(int argc, char** argv) : argc_(argc), argv_(argv) {}

    int argc() const noexcept { return argc_; }
    char** argv() const noexcept { return argv_; }

    App& title(std::string t) {
        title_ = std::move(t);
        return *this;
    }

    App& stylesheet(std::string path) {
        stylesheet_path_ = std::move(path);
        return *this;
    }

    const std::string& stylesheet_path() const noexcept {
        return stylesheet_path_;
    }

    App& theme(std::string th) {
        theme_name_ = std::move(th);
        if (theme_name_ == "dark") {
            ThemeEngine::instance().set_dark_mode(true);
        } else if (theme_name_ == "light") {
            ThemeEngine::instance().set_dark_mode(false);
        }
        return *this;
    }

    App& render(Component root) {
        root_vnode_ = root;
        root_component_factory_ = [root]() { return root; };
        return *this;
    }

    App& render(std::function<Component()> factory) {
        root_component_factory_ = std::move(factory);
        if (root_component_factory_) {
            root_vnode_ = root_component_factory_();
        }
        return *this;
    }

    const Component& root() const noexcept {
        return root_vnode_;
    }

    std::shared_ptr<ComponentInstance> root_instance() const noexcept {
        return root_instance_;
    }

    int run() {
        if (!root_component_factory_) {
            if (root_vnode_.type().empty()) {
                std::cerr << "[PingDartUI App]: Error - no root component provided." << std::endl;
                return 1;
            }
            root_component_factory_ = [this]() { return root_vnode_; };
        }

        // 1. Load stylesheet if present
        if (!stylesheet_path_.empty()) {
            std::ifstream css_file(stylesheet_path_);
            if (css_file) {
                std::ostringstream ss;
                ss << css_file.rdbuf();
                std::vector<std::string> diags;
                auto rules = CssParser::parse(ss.str(), &diags);
                // Stylesheet loaded
            }
        }

        // 2. Build root vnode & mount persistent instance tree
        root_vnode_ = root_component_factory_();
        root_instance_ = Reconciler::mount(root_vnode_, nullptr, 0, "app-root");

        // 3. Connect scheduler to update root instance
        Scheduler::instance().set_update_handler([this](std::shared_ptr<ComponentInstance> dirty_node) {
            if (dirty_node && root_component_factory_) {
                Component next_vnode = root_component_factory_();
                root_instance_ = Reconciler::reconcile(root_instance_, next_vnode, 0, "app-root");
                root_vnode_ = next_vnode;
            }
        });

        is_running_ = true;
        return 0;
    }

    void shutdown() {
        if (root_instance_) {
            root_instance_->unmount();
            root_instance_ = nullptr;
        }
        is_running_ = false;
    }
};

} // namespace pingdart::ui
