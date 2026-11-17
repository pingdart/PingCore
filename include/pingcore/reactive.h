// include/pingcore/reactive.h
// PingCore Reactive State Engine (PKG-126: Production-Hardened, Race-Free Weak Subscriptions & Lifetime Guarantees)
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <iostream>
#include <algorithm>

namespace pingcore {

class ReactiveNode;

// Global Reactive Context for Automatic Dependency Tracking & Batching
class ReactiveContext {
public:
    static ReactiveContext& instance() {
        static ReactiveContext s_instance;
        return s_instance;
    }

    void push_active_node(ReactiveNode* node) {
        tl_active_stack().push_back(node);
    }

    void pop_active_node() {
        auto& stack = tl_active_stack();
        if (!stack.empty()) {
            stack.pop_back();
        }
    }

    ReactiveNode* get_active_node() {
        auto& stack = tl_active_stack();
        return stack.empty() ? nullptr : stack.back();
    }

    void start_batch() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++batch_depth_;
    }

    void end_batch();

    bool is_batching() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return batch_depth_ > 0;
    }

    void queue_dirty_node(ReactiveNode* node);

private:
    static std::vector<ReactiveNode*>& tl_active_stack() {
        thread_local std::vector<ReactiveNode*> s_stack;
        return s_stack;
    }

    mutable std::mutex mutex_;
    int batch_depth_{0};
    std::unordered_set<ReactiveNode*> pending_dirty_nodes_;
};

// Base class for any node participating in the reactive dependency graph
// PKG-126: Inherits from std::enable_shared_from_this for race-free weak subscription ownership
class ReactiveNode : public std::enable_shared_from_this<ReactiveNode> {
public:
    virtual ~ReactiveNode() = default;
    virtual void on_dependency_changed() = 0;
    virtual void track_dependency(ReactiveNode* /*dep*/) {}

    // PKG-126: Shared-pointer subscriber registration (race-free weak reference)
    void add_subscriber(const std::shared_ptr<ReactiveNode>& subscriber) {
        if (!subscriber) return;
        std::lock_guard<std::mutex> lock(node_mutex_);
        // Prune expired
        subscribers_.erase(
            std::remove_if(subscribers_.begin(), subscribers_.end(),
                           [](const std::weak_ptr<ReactiveNode>& w) { return w.expired(); }),
            subscribers_.end());
        // Avoid duplicates
        for (const auto& w : subscribers_) {
            if (auto s = w.lock()) {
                if (s == subscriber) return;
            }
        }
        subscribers_.push_back(subscriber);
    }

    // PKG-126: Backward-compatible overload that detects shared ownership
    void add_subscriber(ReactiveNode* subscriber) {
        if (!subscriber) return;
        try {
            add_subscriber(subscriber->shared_from_this());
        } catch (const std::bad_weak_ptr&) {
            std::lock_guard<std::mutex> lock(node_mutex_);
            raw_subscribers_.insert(subscriber);
        }
    }

    void remove_subscriber(const std::shared_ptr<ReactiveNode>& subscriber) {
        if (!subscriber) return;
        std::lock_guard<std::mutex> lock(node_mutex_);
        subscribers_.erase(
            std::remove_if(subscribers_.begin(), subscribers_.end(),
                           [&](const std::weak_ptr<ReactiveNode>& w) {
                               auto s = w.lock();
                               return !s || s == subscriber;
                           }),
            subscribers_.end());
    }

    void remove_subscriber(ReactiveNode* subscriber) {
        if (!subscriber) return;
        std::lock_guard<std::mutex> lock(node_mutex_);
        subscribers_.erase(
            std::remove_if(subscribers_.begin(), subscribers_.end(),
                           [&](const std::weak_ptr<ReactiveNode>& w) {
                               auto s = w.lock();
                               return !s || s.get() == subscriber;
                           }),
            subscribers_.end());
        raw_subscribers_.erase(subscriber);
    }

    // PKG-126: Hardened notification protocol — locks weak references into strong shared_ptrs
    // during callback execution, guaranteeing objects cannot be freed while callbacks run
    void notify_subscribers() {
        std::vector<std::shared_ptr<ReactiveNode>> strong_subs;
        std::vector<ReactiveNode*> raw_subs;
        {
            std::lock_guard<std::mutex> lock(node_mutex_);
            for (auto it = subscribers_.begin(); it != subscribers_.end();) {
                if (auto s = it->lock()) {
                    if (!s->is_disposed()) {
                        strong_subs.push_back(std::move(s));
                    }
                    ++it;
                } else {
                    it = subscribers_.erase(it);
                }
            }
            raw_subs.assign(raw_subscribers_.begin(), raw_subscribers_.end());
        }

        // Notification occurs with node_mutex_ released to avoid deadlock
        for (const auto& sub : strong_subs) {
            if (sub && !sub->is_disposed()) {
                if (ReactiveContext::instance().is_batching()) {
                    ReactiveContext::instance().queue_dirty_node(sub.get());
                } else {
                    sub->on_dependency_changed();
                }
            }
        }
        for (auto* sub : raw_subs) {
            if (sub && !sub->is_disposed()) {
                if (ReactiveContext::instance().is_batching()) {
                    ReactiveContext::instance().queue_dirty_node(sub);
                } else {
                    sub->on_dependency_changed();
                }
            }
        }
    }

    [[nodiscard]] bool is_disposed() const {
        return disposed_.load(std::memory_order_acquire);
    }

    [[nodiscard]] size_t active_subscriber_count() const {
        std::lock_guard<std::mutex> lock(node_mutex_);
        size_t count = 0;
        for (const auto& w : subscribers_) {
            if (!w.expired()) count++;
        }
        return count + raw_subscribers_.size();
    }

protected:
    void mark_disposed() {
        disposed_.store(true, std::memory_order_release);
    }

    mutable std::mutex node_mutex_;
    std::vector<std::weak_ptr<ReactiveNode>> subscribers_;
    std::unordered_set<ReactiveNode*> raw_subscribers_;
    std::atomic<bool> disposed_{false};
};

// PKG-126: RAII Subscription Handle for explicit unsubscription
class SubscriptionHandle {
public:
    SubscriptionHandle() = default;
    SubscriptionHandle(std::weak_ptr<ReactiveNode> source, std::weak_ptr<ReactiveNode> target)
        : source_(std::move(source)), target_(std::move(target)), active_(true) {}

    ~SubscriptionHandle() {
        unsubscribe();
    }

    SubscriptionHandle(const SubscriptionHandle&) = delete;
    SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

    SubscriptionHandle(SubscriptionHandle&& other) noexcept
        : source_(std::move(other.source_)), target_(std::move(other.target_)), active_(other.active_.load()) {
        other.active_.store(false);
    }

    SubscriptionHandle& operator=(SubscriptionHandle&& other) noexcept {
        if (this != &other) {
            unsubscribe();
            source_ = std::move(other.source_);
            target_ = std::move(other.target_);
            active_.store(other.active_.load());
            other.active_.store(false);
        }
        return *this;
    }

    void unsubscribe() {
        if (active_.exchange(false)) {
            if (auto src = source_.lock()) {
                if (auto tgt = target_.lock()) {
                    src->remove_subscriber(tgt);
                }
            }
        }
    }

    [[nodiscard]] bool is_active() const {
        return active_.load() && !source_.expired() && !target_.expired();
    }

private:
    std::weak_ptr<ReactiveNode> source_;
    std::weak_ptr<ReactiveNode> target_;
    std::atomic<bool> active_{false};
};

inline void ReactiveContext::queue_dirty_node(ReactiveNode* node) {
    if (node && !node->is_disposed()) {
        pending_dirty_nodes_.insert(node);
    }
}

inline void ReactiveContext::end_batch() {
    std::unordered_set<ReactiveNode*> to_notify;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (batch_depth_ > 0) {
            --batch_depth_;
        }
        if (batch_depth_ == 0) {
            to_notify = std::move(pending_dirty_nodes_);
            pending_dirty_nodes_.clear();
        }
    }
    for (auto* node : to_notify) {
        if (node && !node->is_disposed()) {
            node->on_dependency_changed();
        }
    }
}

// Scoped Batch Helper
inline void batch(const std::function<void()>& fn) {
    ReactiveContext::instance().start_batch();
    try {
        fn();
    } catch (...) {
        ReactiveContext::instance().end_batch();
        throw;
    }
    ReactiveContext::instance().end_batch();
}

// --- Signal<T>: Observable Value ---
template <typename T>
class SignalImpl : public ReactiveNode {
public:
    explicit SignalImpl(T initial_val)
        : value_(std::move(initial_val)) {}

    const T& get() {
        ReactiveNode* active = ReactiveContext::instance().get_active_node();
        if (active) {
            add_subscriber(active);
            active->track_dependency(this);
        }
        std::lock_guard<std::mutex> lock(data_mutex_);
        return value_;
    }

    void set(T new_val) {
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if constexpr (requires { value_ == new_val; }) {
                if (value_ == new_val) return;
            }
            value_ = std::move(new_val);
        }
        notify_subscribers();
    }

    void update(std::function<T(const T&)> updater) {
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            value_ = updater(value_);
        }
        notify_subscribers();
    }

    void on_dependency_changed() override {}
    void track_dependency(ReactiveNode*) override {}

private:
    mutable std::mutex data_mutex_;
    T value_;
};

template <typename T>
class Signal {
public:
    Signal() : impl_(std::make_shared<SignalImpl<T>>(T{})) {}
    explicit Signal(T initial) : impl_(std::make_shared<SignalImpl<T>>(std::move(initial))) {}

    [[nodiscard]] const T& get() const { return impl_->get(); }
    [[nodiscard]] const T& operator()() const { return impl_->get(); }

    void set(T val) const { impl_->set(std::move(val)); }
    void operator=(T val) const { impl_->set(std::move(val)); }

    void update(std::function<T(const T&)> updater) const {
        impl_->update(std::move(updater));
    }

    [[nodiscard]] std::shared_ptr<SignalImpl<T>> impl() const { return impl_; }

    // PKG-126: Create an explicit subscription handle between this signal and a target node
    [[nodiscard]] SubscriptionHandle subscribe(const std::shared_ptr<ReactiveNode>& target) const {
        if (!impl_ || !target) return {};
        impl_->add_subscriber(target);
        return SubscriptionHandle(impl_, target);
    }

private:
    std::shared_ptr<SignalImpl<T>> impl_;
};

template <typename T>
inline Signal<T> useSignal(T initial) {
    return Signal<T>(std::move(initial));
}

// --- Computed<T>: Automatically Derived Value ---
template <typename T>
class ComputedImpl : public ReactiveNode {
public:
    explicit ComputedImpl(std::function<T()> compute_fn)
        : compute_fn_(std::move(compute_fn)), dirty_(true) {}

    ~ComputedImpl() override {
        dispose();
    }

    const T& get() {
        ReactiveNode* active = ReactiveContext::instance().get_active_node();
        if (active) {
            add_subscriber(active);
            active->track_dependency(this);
        }

        std::lock_guard<std::mutex> lock(data_mutex_);
        if (dirty_) {
            recompute();
        }
        return cached_value_;
    }

    void on_dependency_changed() override {
        if (is_disposed()) return;
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            dirty_ = true;
        }
        notify_subscribers();
    }

    // PKG-126: Race-free dependency unsubscription
    void dispose() {
        if (disposed_flag_.exchange(true)) return;
        mark_disposed();
        std::vector<std::weak_ptr<ReactiveNode>> weak_deps;
        std::unordered_set<ReactiveNode*> raw_deps;
        {
            std::lock_guard<std::mutex> lock(dep_mutex_);
            weak_deps = std::move(weak_dependencies_);
            raw_deps = std::move(raw_dependencies_);
            weak_dependencies_.clear();
            raw_dependencies_.clear();
        }
        for (auto& w : weak_deps) {
            if (auto dep = w.lock()) {
                dep->remove_subscriber(this);
            }
        }
        for (auto* dep : raw_deps) {
            if (dep) {
                dep->remove_subscriber(this);
            }
        }
    }

    void track_dependency(ReactiveNode* dep) override {
        if (!dep) return;
        try {
            auto sp = dep->shared_from_this();
            std::lock_guard<std::mutex> lock(dep_mutex_);
            for (const auto& w : weak_dependencies_) {
                if (auto s = w.lock()) {
                    if (s == sp) return;
                }
            }
            weak_dependencies_.push_back(sp);
        } catch (const std::bad_weak_ptr&) {
            std::lock_guard<std::mutex> lock(dep_mutex_);
            raw_dependencies_.insert(dep);
        }
    }

private:
    void recompute() {
        // Clear old dependencies before re-tracking
        {
            std::lock_guard<std::mutex> lock(dep_mutex_);
            for (auto& w : weak_dependencies_) {
                if (auto dep = w.lock()) {
                    dep->remove_subscriber(this);
                }
            }
            weak_dependencies_.clear();
            for (auto* dep : raw_dependencies_) {
                if (dep) {
                    dep->remove_subscriber(this);
                }
            }
            raw_dependencies_.clear();
        }
        ReactiveContext::instance().push_active_node(this);
        cached_value_ = compute_fn_();
        ReactiveContext::instance().pop_active_node();
        dirty_ = false;
    }

    mutable std::mutex data_mutex_;
    mutable std::mutex dep_mutex_;
    std::function<T()> compute_fn_;
    T cached_value_{};
    bool dirty_{true};
    std::atomic<bool> disposed_flag_{false};
    std::vector<std::weak_ptr<ReactiveNode>> weak_dependencies_;
    std::unordered_set<ReactiveNode*> raw_dependencies_;
};

template <typename T>
class Computed {
public:
    explicit Computed(std::function<T()> fn)
        : impl_(std::make_shared<ComputedImpl<T>>(std::move(fn))) {}

    ~Computed() = default;

    [[nodiscard]] const T& get() const { return impl_->get(); }
    [[nodiscard]] const T& operator()() const { return impl_->get(); }

    void dispose() { impl_->dispose(); }

private:
    std::shared_ptr<ComputedImpl<T>> impl_;
};

template <typename T>
inline Computed<T> useComputed(std::function<T()> compute_fn) {
    return Computed<T>(std::move(compute_fn));
}

// --- Effect: Controlled Reactive Side Effects ---
class Effect : public ReactiveNode {
public:
    explicit Effect(std::function<void()> effect_fn)
        : effect_fn_(std::move(effect_fn)) {}

    ~Effect() override {
        dispose();
    }

    void run() {
        if (is_disposed()) return;
        thread_local int tl_effect_depth = 0;
        constexpr int kMaxEffectDepth = 50;
        if (tl_effect_depth >= kMaxEffectDepth) {
            // PKG-127: Guard against infinite cascading effect cycles
            return;
        }

        thread_local std::unordered_set<Effect*> tl_running_effects;
        if (tl_running_effects.find(this) != tl_running_effects.end()) {
            // PKG-127: Prevent recursive deadlock on non-recursive run_mutex_
            return;
        }

        std::lock_guard<std::mutex> lock(run_mutex_);
        if (is_disposed()) return;

        struct RunningGuard {
            std::unordered_set<Effect*>& set;
            Effect* eff;
            RunningGuard(std::unordered_set<Effect*>& s, Effect* e) : set(s), eff(e) { set.insert(eff); }
            ~RunningGuard() { set.erase(eff); }
        } running_guard(tl_running_effects, this);

        // Clear old dependencies before re-tracking
        {
            std::lock_guard<std::mutex> lock2(dep_mutex_);
            for (auto& w : weak_dependencies_) {
                if (auto dep = w.lock()) {
                    dep->remove_subscriber(this);
                }
            }
            weak_dependencies_.clear();
            for (auto* dep : raw_dependencies_) {
                if (dep) {
                    dep->remove_subscriber(this);
                }
            }
            raw_dependencies_.clear();
        }

        ReactiveContext::instance().push_active_node(this);
        tl_effect_depth++;
        try {
            effect_fn_();
        } catch (...) {
            tl_effect_depth--;
            ReactiveContext::instance().pop_active_node();
            throw;
        }
        tl_effect_depth--;
        ReactiveContext::instance().pop_active_node();
    }

    void on_dependency_changed() override {
        if (is_disposed()) return;
        run();
    }

    void dispose() {
        if (disposed_flag_.exchange(true)) return;
        mark_disposed();
        std::vector<std::weak_ptr<ReactiveNode>> weak_deps;
        std::unordered_set<ReactiveNode*> raw_deps;
        {
            std::lock_guard<std::mutex> lock(dep_mutex_);
            weak_deps = std::move(weak_dependencies_);
            raw_deps = std::move(raw_dependencies_);
            weak_dependencies_.clear();
            raw_dependencies_.clear();
        }
        for (auto& w : weak_deps) {
            if (auto dep = w.lock()) {
                dep->remove_subscriber(this);
            }
        }
        for (auto* dep : raw_deps) {
            if (dep) {
                dep->remove_subscriber(this);
            }
        }
    }

    [[nodiscard]] bool is_effect_disposed() const {
        return disposed_flag_.load(std::memory_order_acquire);
    }

    void track_dependency(ReactiveNode* dep) override {
        if (!dep) return;
        try {
            auto sp = dep->shared_from_this();
            std::lock_guard<std::mutex> lock(dep_mutex_);
            for (const auto& w : weak_dependencies_) {
                if (auto s = w.lock()) {
                    if (s == sp) return;
                }
            }
            weak_dependencies_.push_back(sp);
        } catch (const std::bad_weak_ptr&) {
            std::lock_guard<std::mutex> lock(dep_mutex_);
            raw_dependencies_.insert(dep);
        }
    }

private:
    mutable std::mutex run_mutex_;
    mutable std::mutex dep_mutex_;
    std::function<void()> effect_fn_;
    std::atomic<bool> disposed_flag_{false};
    std::vector<std::weak_ptr<ReactiveNode>> weak_dependencies_;
    std::unordered_set<ReactiveNode*> raw_dependencies_;
};

inline std::shared_ptr<Effect> create_effect(std::function<void()> fn) {
    auto eff = std::make_shared<Effect>(std::move(fn));
    eff->run();
    return eff;
}

inline std::shared_ptr<Effect> useEffect(std::function<void()> fn) {
    return create_effect(std::move(fn));
}

// ============================================================================
// PKG-127: ReactiveScheduler for UI-Thread / EventLoop Dispatching
// ============================================================================
class ReactiveScheduler {
public:
    static ReactiveScheduler& instance() {
        static ReactiveScheduler s_instance;
        return s_instance;
    }

    void set_ui_dispatcher(std::function<void(std::function<void()>)> dispatcher) {
        std::lock_guard<std::mutex> lock(mutex_);
        ui_dispatcher_ = std::move(dispatcher);
    }

    void queue_task(std::function<void()> task) {
        std::function<void(std::function<void()>)> d;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            d = ui_dispatcher_;
        }
        if (d) {
            d(std::move(task));
        } else {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push_back(std::move(task));
        }
    }

    size_t run_pending_tasks() {
        std::vector<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            batch.swap(task_queue_);
        }
        for (const auto& task : batch) {
            if (task) task();
        }
        return batch.size();
    }

    [[nodiscard]] size_t pending_task_count() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::lock_guard<std::mutex> qlock(queue_mutex_);
        ui_dispatcher_ = nullptr;
        task_queue_.clear();
    }

private:
    mutable std::mutex mutex_;
    mutable std::mutex queue_mutex_;
    std::function<void(std::function<void()>)> ui_dispatcher_;
    std::vector<std::function<void()>> task_queue_;
};

} // namespace pingcore
