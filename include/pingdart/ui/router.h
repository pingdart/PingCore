// include/pingdart/ui/router.h
// PingDartUI Client-Side Next.js-Style Router (PKG-135)
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace pingdart::ui {

class Router {
public:
    static Router& instance() {
        static Router s_router;
        return s_router;
    }

    static void push(const std::string& path) {
        instance().navigate_to(path);
    }

    static void replace(const std::string& path) {
        instance().replace_to(path);
    }

    static void back() {
        instance().navigate_back();
    }

    static std::string current_path() {
        return instance().path_;
    }

    static void on_route_change(std::function<void(const std::string&)> cb) {
        instance().callbacks_.push_back(std::move(cb));
    }

    static void reset() {
        instance().path_ = "/";
        instance().history_.clear();
        instance().history_.push_back("/");
        instance().callbacks_.clear();
    }

private:
    Router() {
        path_ = "/";
        history_.push_back(path_);
    }

    void navigate_to(const std::string& path) {
        path_ = path;
        history_.push_back(path_);
        notify();
    }

    void replace_to(const std::string& path) {
        path_ = path;
        if (!history_.empty()) {
            history_.back() = path_;
        } else {
            history_.push_back(path_);
        }
        notify();
    }

    void navigate_back() {
        if (history_.size() > 1) {
            history_.pop_back();
            path_ = history_.back();
            notify();
        }
    }

    void notify() {
        for (const auto& cb : callbacks_) {
            if (cb) cb(path_);
        }
    }

    std::string path_{"/"};
    std::vector<std::string> history_{};
    std::vector<std::function<void(const std::string&)>> callbacks_{};
};

} // namespace pingdart::ui
