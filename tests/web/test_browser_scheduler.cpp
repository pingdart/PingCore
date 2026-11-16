#include <gtest/gtest.h>
#include <pingdart/web/browser_scheduler.h>
#include "../../src/web/browser/browser_scheduler.h"
#include <chrono>
#include <thread>

using namespace pingdart::web;

TEST(BrowserSchedulerTest, NonBlockingMicrotaskDrainage) {
    BrowserScheduler scheduler;

    std::vector<int> execution_order;

    scheduler.queue_microtask([&]() { execution_order.push_back(1); });
    scheduler.queue_microtask([&]() { execution_order.push_back(2); });
    scheduler.queue_microtask([&]() { execution_order.push_back(3); });

    EXPECT_TRUE(scheduler.has_pending_tasks());
    EXPECT_EQ(scheduler.pending_microtask_count(), 3u);

    // Non-blocking drainage
    size_t executed = scheduler.drain_microtasks();
    EXPECT_EQ(executed, 3u);
    EXPECT_FALSE(scheduler.has_pending_tasks());

    ASSERT_EQ(execution_order.size(), 3u);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 2);
    EXPECT_EQ(execution_order[2], 3);
}

TEST(BrowserSchedulerTest, RequestAnimationFrameCycle) {
    BrowserScheduler scheduler;

    double frame_timestamp = -1.0;
    bool frame_executed = false;

    scheduler.request_animation_frame([&](double ts) {
        frame_timestamp = ts;
        frame_executed = true;
    });

    EXPECT_TRUE(scheduler.has_pending_tasks());
    EXPECT_EQ(scheduler.pending_frame_count(), 1u);

    // Single step frame (16.6 ms timestamp)
    bool stepped = scheduler.step_frame(16.667);
    EXPECT_TRUE(stepped);
    EXPECT_TRUE(frame_executed);
    EXPECT_DOUBLE_EQ(frame_timestamp, 16.667);
    EXPECT_FALSE(scheduler.has_pending_tasks());

    // Subsequent step with empty queue returns false immediately
    bool empty_step = scheduler.step_frame(33.333);
    EXPECT_FALSE(empty_step);
}

TEST(BrowserSchedulerTest, DelayedTimerTaskExecution) {
    BrowserScheduler scheduler;

    bool timer_executed = false;
    scheduler.set_timeout(10, [&]() {
        timer_executed = true;
    });

    // Step immediately before deadline - should not execute
    scheduler.step_frame(0.0);
    EXPECT_FALSE(timer_executed);

    // Sleep past deadline and step frame
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    scheduler.step_frame(15.0);
    EXPECT_TRUE(timer_executed);
}
