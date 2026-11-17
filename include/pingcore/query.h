// include/pingcore/query.h
// PingCore Full-Stack Reactive Server Data SDK (PKG-124: Query Caching, Invalidation & Real-Time Sync)
#pragma once

#include <pingcore/reactive.h>
#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>

namespace pingcore {

template <typename T>
struct QueryDataState {
    T data{};
    bool loading{true};
    std::string error;
    uint64_t fetched_at{0};

    bool operator==(const QueryDataState&) const = default;
};

class QueryCache {
public:
    static QueryCache& instance() {
        static QueryCache s_instance;
        return s_instance;
    }

    void invalidate(std::string_view key) {
        std::function<void()> refetch_cb;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = invalidators_.find(std::string(key));
            if (it != invalidators_.end()) {
                refetch_cb = it->second;
            }
        }
        if (refetch_cb) {
            refetch_cb();
        }
    }

    void register_invalidator(const std::string& key, std::function<void()> refetch_fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidators_[key] = std::move(refetch_fn);
    }

    void unregister_invalidator(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidators_.erase(key);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        invalidators_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::function<void()>> invalidators_;
};

inline void invalidate_query(std::string_view key) {
    QueryCache::instance().invalidate(key);
}

template <typename T>
class Query {
public:
    Query(std::string key, std::function<T()> fetcher)
        : key_(std::move(key)),
          fetcher_(std::move(fetcher)),
          state_signal_(useSignal<QueryDataState<T>>({T{}, true, "", 0})) {

        // Register refetcher with global query cache
        QueryCache::instance().register_invalidator(key_, [this]() {
            this->refetch();
        });

        // Trigger initial fetch
        execute_fetch();
    }

    ~Query() {
        QueryCache::instance().unregister_invalidator(key_);
    }

    [[nodiscard]] const T& data() const {
        return state_signal_.get().data;
    }

    [[nodiscard]] bool loading() const {
        return state_signal_.get().loading;
    }

    [[nodiscard]] const std::string& error() const {
        return state_signal_.get().error;
    }

    [[nodiscard]] bool is_success() const {
        const auto& s = state_signal_.get();
        return !s.loading && s.error.empty();
    }

    void refetch() {
        execute_fetch();
    }

    void set_data(T new_data) {
        auto now = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
        state_signal_.set({std::move(new_data), false, "", now});
    }

    [[nodiscard]] const std::string& key() const noexcept {
        return key_;
    }

private:
    void execute_fetch() {
        state_signal_.update([](const QueryDataState<T>& prev) {
            QueryDataState<T> next = prev;
            next.loading = true;
            next.error = "";
            return next;
        });

        try {
            T result = fetcher_();
            auto now = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
            state_signal_.set({std::move(result), false, "", now});
        } catch (const std::exception& ex) {
            state_signal_.update([&ex](const QueryDataState<T>& prev) {
                QueryDataState<T> next = prev;
                next.loading = false;
                next.error = ex.what();
                return next;
            });
        } catch (...) {
            state_signal_.update([](const QueryDataState<T>& prev) {
                QueryDataState<T> next = prev;
                next.loading = false;
                next.error = "Unknown query execution failure";
                return next;
            });
        }
    }

    std::string key_;
    std::function<T()> fetcher_;
    Signal<QueryDataState<T>> state_signal_;
};

template <typename T>
inline std::shared_ptr<Query<T>> useQuery(std::string key, std::function<T()> fetcher) {
    return std::make_shared<Query<T>>(std::move(key), std::move(fetcher));
}

} // namespace pingcore
