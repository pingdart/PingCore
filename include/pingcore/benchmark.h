// include/pingcore/benchmark.h
// PingCore Performance Benchmark Infrastructure (PKG-125)
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <functional>
#include <numeric>
#include <cmath>

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

namespace pingcore {

// High-resolution benchmark timer with percentile calculation
class BenchmarkTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::nanoseconds;

    void start() {
        start_ = Clock::now();
    }

    Duration stop() {
        auto end = Clock::now();
        auto elapsed = std::chrono::duration_cast<Duration>(end - start_);
        samples_.push_back(elapsed);
        return elapsed;
    }

    // Run a function N times and record each duration
    void run(size_t iterations, const std::function<void()>& fn) {
        samples_.reserve(samples_.size() + iterations);
        for (size_t i = 0; i < iterations; ++i) {
            auto t0 = Clock::now();
            fn();
            auto t1 = Clock::now();
            samples_.push_back(std::chrono::duration_cast<Duration>(t1 - t0));
        }
    }

    [[nodiscard]] size_t sample_count() const { return samples_.size(); }

    [[nodiscard]] Duration percentile(double p) const {
        if (samples_.empty()) return Duration{0};

        auto sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        double index = p / 100.0 * static_cast<double>(sorted.size() - 1);
        auto idx = static_cast<size_t>(index);
        if (idx >= sorted.size() - 1) return sorted.back();
        return sorted[idx];
    }

    [[nodiscard]] Duration p50() const { return percentile(50); }
    [[nodiscard]] Duration p95() const { return percentile(95); }
    [[nodiscard]] Duration p99() const { return percentile(99); }

    [[nodiscard]] Duration min() const {
        if (samples_.empty()) return Duration{0};
        return *std::min_element(samples_.begin(), samples_.end());
    }

    [[nodiscard]] Duration max() const {
        if (samples_.empty()) return Duration{0};
        return *std::max_element(samples_.begin(), samples_.end());
    }

    [[nodiscard]] Duration mean() const {
        if (samples_.empty()) return Duration{0};
        int64_t total = 0;
        for (const auto& s : samples_) total += s.count();
        return Duration{total / static_cast<int64_t>(samples_.size())};
    }

    [[nodiscard]] Duration total() const {
        int64_t sum = 0;
        for (const auto& s : samples_) sum += s.count();
        return Duration{sum};
    }

    void clear() { samples_.clear(); }

private:
    Clock::time_point start_;
    std::vector<Duration> samples_;
};

// Benchmark report with named metrics
struct BenchmarkReport {
    struct Metric {
        std::string name;
        size_t sample_count{0};
        int64_t p50_ns{0};
        int64_t p95_ns{0};
        int64_t p99_ns{0};
        int64_t min_ns{0};
        int64_t max_ns{0};
        int64_t mean_ns{0};
    };

    std::string suite_name;
    std::vector<Metric> metrics;

    void add_metric(const std::string& name, const BenchmarkTimer& timer) {
        metrics.push_back({
            name,
            timer.sample_count(),
            timer.p50().count(),
            timer.p95().count(),
            timer.p99().count(),
            timer.min().count(),
            timer.max().count(),
            timer.mean().count()
        });
    }

    [[nodiscard]] std::string format() const {
        std::ostringstream ss;
        ss << "=== Benchmark Report: " << suite_name << " ===\n";
        ss << std::left << std::setw(35) << "Metric"
           << std::right << std::setw(10) << "Samples"
           << std::setw(12) << "P50 (ns)"
           << std::setw(12) << "P95 (ns)"
           << std::setw(12) << "P99 (ns)"
           << std::setw(12) << "Min (ns)"
           << std::setw(12) << "Max (ns)"
           << std::setw(12) << "Mean (ns)"
           << "\n";
        ss << std::string(107, '-') << "\n";
        for (const auto& m : metrics) {
            ss << std::left << std::setw(35) << m.name
               << std::right << std::setw(10) << m.sample_count
               << std::setw(12) << m.p50_ns
               << std::setw(12) << m.p95_ns
               << std::setw(12) << m.p99_ns
               << std::setw(12) << m.min_ns
               << std::setw(12) << m.max_ns
               << std::setw(12) << m.mean_ns
               << "\n";
        }
        return ss.str();
    }

    // Check if a metric is within a threshold (p99 < threshold_ns)
    [[nodiscard]] bool check_threshold(const std::string& metric_name, int64_t threshold_ns) const {
        for (const auto& m : metrics) {
            if (m.name == metric_name) {
                return m.p99_ns <= threshold_ns;
            }
        }
        return false;
    }
};

// Memory usage tracker (platform-specific)
class MemoryTracker {
public:
    struct MemorySnapshot {
        size_t resident_bytes{0};
        size_t virtual_bytes{0};
    };

    static MemorySnapshot current() {
        MemorySnapshot snap;

#ifdef __APPLE__
        mach_task_basic_info_data_t info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
            snap.resident_bytes = info.resident_size;
            snap.virtual_bytes = info.virtual_size;
        }
#elif defined(__linux__)
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            snap.resident_bytes = static_cast<size_t>(usage.ru_maxrss) * 1024;
        }
#endif

        return snap;
    }

    void record() {
        snapshots_.push_back(current());
    }

    // Check for memory growth: compares first N snapshots average to last N
    [[nodiscard]] bool has_growth(size_t window = 10, double threshold_ratio = 1.1) const {
        if (snapshots_.size() < window * 2) return false;

        size_t first_avg = 0, last_avg = 0;
        for (size_t i = 0; i < window; ++i) {
            first_avg += snapshots_[i].resident_bytes;
            last_avg += snapshots_[snapshots_.size() - window + i].resident_bytes;
        }
        first_avg /= window;
        last_avg /= window;

        if (first_avg == 0) return false;
        return (static_cast<double>(last_avg) / static_cast<double>(first_avg)) > threshold_ratio;
    }

    [[nodiscard]] size_t snapshot_count() const { return snapshots_.size(); }

    [[nodiscard]] size_t peak_resident() const {
        size_t peak = 0;
        for (const auto& s : snapshots_) {
            peak = std::max(peak, s.resident_bytes);
        }
        return peak;
    }

    void clear() { snapshots_.clear(); }

private:
    std::vector<MemorySnapshot> snapshots_;
};

// Frame time benchmark for UI rendering
class FrameTimeBenchmark {
public:
    void record_frame(std::chrono::nanoseconds frame_time) {
        timer_samples_.push_back(frame_time);
    }

    void simulate_frame(const std::function<void()>& render_fn) {
        auto t0 = std::chrono::high_resolution_clock::now();
        render_fn();
        auto t1 = std::chrono::high_resolution_clock::now();
        timer_samples_.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0));
    }

    [[nodiscard]] size_t frame_count() const { return timer_samples_.size(); }

    // Calculate effective FPS from mean frame time
    [[nodiscard]] double average_fps() const {
        if (timer_samples_.empty()) return 0.0;
        int64_t total = 0;
        for (const auto& s : timer_samples_) total += s.count();
        double mean_seconds = static_cast<double>(total) / static_cast<double>(timer_samples_.size()) / 1e9;
        return mean_seconds > 0 ? 1.0 / mean_seconds : 0.0;
    }

    // Check if 95th percentile frame time is under target (e.g., 16.67ms for 60fps)
    [[nodiscard]] bool meets_target_fps(double target_fps) const {
        if (timer_samples_.empty()) return false;
        auto sorted = timer_samples_;
        std::sort(sorted.begin(), sorted.end());
        double index = 0.95 * static_cast<double>(sorted.size() - 1);
        auto p95 = sorted[static_cast<size_t>(index)];
        double target_ns = 1e9 / target_fps;
        return static_cast<double>(p95.count()) <= target_ns;
    }

    void clear() { timer_samples_.clear(); }

private:
    std::vector<std::chrono::nanoseconds> timer_samples_;
};

// Throughput benchmark for server request processing
class ThroughputBenchmark {
public:
    struct Result {
        size_t total_requests{0};
        std::chrono::nanoseconds total_time{0};
        double requests_per_second{0.0};
        double avg_latency_us{0.0};
    };

    Result run(size_t total_requests, const std::function<void()>& request_fn) {
        Result result;
        result.total_requests = total_requests;

        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < total_requests; ++i) {
            request_fn();
        }
        auto t1 = std::chrono::high_resolution_clock::now();

        result.total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
        double total_seconds = static_cast<double>(result.total_time.count()) / 1e9;
        result.requests_per_second = total_seconds > 0 ? static_cast<double>(total_requests) / total_seconds : 0;
        result.avg_latency_us = total_seconds > 0 ? (total_seconds / static_cast<double>(total_requests)) * 1e6 : 0;

        return result;
    }
};

} // namespace pingcore
