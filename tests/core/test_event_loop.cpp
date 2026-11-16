// tests/core/test_event_loop.cpp
#include <gtest/gtest.h>
#include <pingdart/core/event_loop.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace pingdart::core;
using namespace std::chrono_literals;

TEST(EventLoopTest, CreationAndThreadCheck) {
    auto loop = EventLoop::create();
    ASSERT_NE(loop, nullptr);
    EXPECT_TRUE(loop->is_on_loop_thread());
    EXPECT_FALSE(loop->is_running());
    EXPECT_FALSE(loop->next_deadline().has_value());
}

TEST(EventLoopTest, HasReadyTasksStateCheck) {
    auto loop = EventLoop::create();
    EXPECT_FALSE(loop->has_ready_tasks());

    loop->post_task([]() {});
    EXPECT_TRUE(loop->has_ready_tasks());
    loop->run_one();
    EXPECT_FALSE(loop->has_ready_tasks());

    loop->post_microtask([]() {});
    EXPECT_TRUE(loop->has_ready_tasks());
    loop->run_one();
    EXPECT_FALSE(loop->has_ready_tasks());

    loop->post_delayed_task([]() {}, 50ms);
    EXPECT_FALSE(loop->has_ready_tasks()); // Not yet ready
}

TEST(EventLoopTest, MacrotaskExecutionInOrder) {
    auto loop = EventLoop::create();
    std::vector<int> executed;

    loop->post_task([&]() { executed.push_back(1); });
    loop->post_task([&]() { executed.push_back(2); });
    loop->post_task([&]() { executed.push_back(3); });

    EXPECT_TRUE(loop->run_one());
    EXPECT_TRUE(loop->run_one());
    EXPECT_TRUE(loop->run_one());
    EXPECT_FALSE(loop->run_one());

    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(executed, expected);
}

TEST(EventLoopTest, MicrotasksExecuteBeforeMacrotasks) {
    auto loop = EventLoop::create();
    std::vector<std::string> log;

    loop->post_task([&]() {
        log.push_back("macrotask 1");
        loop->post_microtask([&]() { log.push_back("microtask from macro 1"); });
    });

    loop->post_task([&]() {
        log.push_back("macrotask 2");
    });

    loop->post_microtask([&]() {
        log.push_back("initial microtask");
    });

    loop->run_one();
    loop->run_one();
    loop->run_one();

    std::vector<std::string> expected = {
        "initial microtask",
        "macrotask 1",
        "microtask from macro 1",
        "macrotask 2"
    };
    EXPECT_EQ(log, expected);
}

TEST(EventLoopTest, NestedMicrotaskChaining) {
    auto loop = EventLoop::create();
    std::vector<int> execution_order;

    loop->post_task([&]() {
        execution_order.push_back(100);
    });
    loop->post_task([&]() {
        execution_order.push_back(200);
    });

    loop->post_microtask([&]() {
        execution_order.push_back(1);
        loop->post_microtask([&]() {
            execution_order.push_back(2);
            loop->post_microtask([&]() {
                execution_order.push_back(3);
            });
        });
    });

    // Turn 1 drains nested microtasks (1, 2, 3) and executes first macrotask (100)
    EXPECT_TRUE(loop->run_one());
    // Turn 2 executes second macrotask (200)
    EXPECT_TRUE(loop->run_one());
    // Turn 3 has no tasks left
    EXPECT_FALSE(loop->run_one());

    std::vector<int> expected = {1, 2, 3, 100, 200};
    EXPECT_EQ(execution_order, expected);
}

TEST(EventLoopTest, DelayedTaskOrderingAndTiming) {
    auto loop = EventLoop::create();
    std::vector<int> executed;

    loop->post_delayed_task([&]() { executed.push_back(30); }, 30ms);
    loop->post_delayed_task([&]() { executed.push_back(10); }, 10ms);
    loop->post_delayed_task([&]() { executed.push_back(20); }, 20ms);

    EXPECT_TRUE(loop->next_deadline().has_value());

    EXPECT_FALSE(loop->run_one(0ms));
    EXPECT_TRUE(executed.empty());

    EXPECT_TRUE(loop->run_one(50ms));
    ASSERT_EQ(executed.size(), 1u);
    EXPECT_EQ(executed[0], 10);

    EXPECT_TRUE(loop->run_one(50ms));
    ASSERT_EQ(executed.size(), 2u);
    EXPECT_EQ(executed[1], 20);

    EXPECT_TRUE(loop->run_one(50ms));
    ASSERT_EQ(executed.size(), 3u);
    EXPECT_EQ(executed[2], 30);

    EXPECT_FALSE(loop->next_deadline().has_value());
}

TEST(EventLoopTest, EarlierDeadlineWhileSleepingWakesUpPromptly) {
    auto loop = EventLoop::create();
    std::atomic<bool> short_timer_ran{false};
    std::atomic<bool> long_timer_ran{false};

    // Schedule long timer (2000ms)
    loop->post_delayed_task([&]() {
        long_timer_ran.store(true);
    }, 2000ms);

    std::thread background_poster([&]() {
        // Sleep briefly, then post a much earlier timer (15ms) while the loop is waiting
        std::this_thread::sleep_for(20ms);
        loop->post_delayed_task([&]() {
            short_timer_ran.store(true);
        }, 15ms);
    });

    auto start = Clock::now();
    // Run turns until short timer runs
    while (!short_timer_ran.load()) {
        loop->run_one(100ms);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);

    background_poster.join();
    EXPECT_TRUE(short_timer_ran.load());
    EXPECT_FALSE(long_timer_ran.load());
    // Should have woken up and executed within well under 1000ms (typically ~40ms)
    EXPECT_LT(elapsed.count(), 1000);
}

TEST(EventLoopTest, WakeupHandleNotification) {
    class MockWakeupHandle : public IWakeupHandle {
    public:
        std::atomic<int> wake_calls{0};
        void wake() override {
            wake_calls.fetch_add(1);
        }
    };

    MockWakeupHandle mock_handle;
    auto loop = EventLoop::create();
    loop->set_wakeup_handle(&mock_handle);
    EXPECT_EQ(loop->wakeup_handle(), &mock_handle);

    loop->post_task([]() {});
    EXPECT_EQ(mock_handle.wake_calls.load(), 1);

    loop->post_microtask([]() {});
    EXPECT_EQ(mock_handle.wake_calls.load(), 2);

    loop->post_delayed_task([]() {}, 50ms);
    EXPECT_EQ(mock_handle.wake_calls.load(), 3);

    loop->terminate(0);
    EXPECT_EQ(mock_handle.wake_calls.load(), 4);
}

TEST(EventLoopTest, TerminateFromWithinCallback) {
    auto loop = EventLoop::create();
    std::atomic<int> completed_tasks{0};

    std::thread runner([&]() {
        int code = loop->run();
        EXPECT_EQ(code, 99);
    });

    loop->post_task([&]() {
        completed_tasks.fetch_add(1);
        // Self-termination from within the running task
        loop->terminate(99);
    });

    // This subsequent task should not execute if terminate stops the loop
    runner.join();
    EXPECT_EQ(completed_tasks.load(), 1);
    EXPECT_FALSE(loop->is_running());
}

TEST(EventLoopTest, ConcurrentPostingFromMultipleThreads) {
    auto loop = EventLoop::create();
    constexpr int kNumThreads = 4;
    constexpr int kTasksPerThread = 250;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kTasksPerThread; ++i) {
                loop->post_task([&]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    int executed_count = 0;
    while (loop->run_one(0ms)) {
        ++executed_count;
    }

    EXPECT_EQ(executed_count, kNumThreads * kTasksPerThread);
    EXPECT_EQ(counter.load(), kNumThreads * kTasksPerThread);
}
