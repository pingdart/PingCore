// tests/core/test_ref_ptr.cpp
#include <gtest/gtest.h>
#include <pingdart/core/ref_ptr.h>
#include <atomic>
#include <vector>
#include <thread>

using namespace pingdart::core;

namespace {

struct DestructorTracker {
    static inline std::atomic<int> alive_count{0};
    static inline std::atomic<int> total_destroyed{0};

    static void reset() {
        alive_count.store(0);
        total_destroyed.store(0);
    }
};

class TrackedNode : public RefCounted<TrackedNode> {
public:
    int id{0};

    explicit TrackedNode(int node_id) : id(node_id) {
        DestructorTracker::alive_count.fetch_add(1);
    }

    ~TrackedNode() override {
        DestructorTracker::alive_count.fetch_sub(1);
        DestructorTracker::total_destroyed.fetch_add(1);
    }
};

class BaseClass : public RefCounted<BaseClass> {
public:
    virtual ~BaseClass() = default;
    virtual std::string type() const { return "Base"; }
};

class DerivedClass : public BaseClass {
public:
    std::string type() const override { return "Derived"; }
};

} // namespace

class RefPtrTest : public ::testing::Test {
protected:
    void SetUp() override {
        DestructorTracker::reset();
    }
};

TEST_F(RefPtrTest, BasicLifecycle) {
    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
    {
        RefPtr<TrackedNode> ptr = make_ref<TrackedNode>(42);
        ASSERT_TRUE(ptr);
        EXPECT_EQ(ptr->id, 42);
        EXPECT_EQ(ptr->ref_count(), 1u);
        EXPECT_EQ(DestructorTracker::alive_count.load(), 1);

        {
            RefPtr<TrackedNode> ptr2 = ptr;
            EXPECT_EQ(ptr->ref_count(), 2u);
            EXPECT_EQ(ptr2->ref_count(), 2u);
            EXPECT_EQ(ptr.get(), ptr2.get());
        }

        // ptr2 went out of scope
        EXPECT_EQ(ptr->ref_count(), 1u);
        EXPECT_EQ(DestructorTracker::alive_count.load(), 1);
    }

    // ptr went out of scope, object should be destroyed
    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
    EXPECT_EQ(DestructorTracker::total_destroyed.load(), 1);
}

TEST_F(RefPtrTest, MoveSemantics) {
    {
        RefPtr<TrackedNode> ptr1 = make_ref<TrackedNode>(100);
        EXPECT_EQ(ptr1->ref_count(), 1u);

        RefPtr<TrackedNode> ptr2(std::move(ptr1));
        EXPECT_FALSE(ptr1);
        ASSERT_TRUE(ptr2);
        EXPECT_EQ(ptr2->id, 100);
        EXPECT_EQ(ptr2->ref_count(), 1u);

        RefPtr<TrackedNode> ptr3;
        ptr3 = std::move(ptr2);
        EXPECT_FALSE(ptr2);
        ASSERT_TRUE(ptr3);
        EXPECT_EQ(ptr3->id, 100);
        EXPECT_EQ(ptr3->ref_count(), 1u);
    }
    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
}

TEST_F(RefPtrTest, ResetAndDetach) {
    RefPtr<TrackedNode> ptr = make_ref<TrackedNode>(200);
    EXPECT_EQ(DestructorTracker::alive_count.load(), 1);

    TrackedNode* raw = ptr.detach();
    EXPECT_FALSE(ptr);
    EXPECT_EQ(DestructorTracker::alive_count.load(), 1);
    EXPECT_EQ(raw->ref_count(), 1u);

    // Re-adopt detached pointer without incrementing ref count
    RefPtr<TrackedNode> adopted = adopt_ref(raw);
    EXPECT_EQ(adopted->ref_count(), 1u);

    // Resetting deletes
    adopted.reset();
    EXPECT_FALSE(adopted);
    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
}

TEST_F(RefPtrTest, PolymorphicConversion) {
    RefPtr<DerivedClass> derived = make_ref<DerivedClass>();
    EXPECT_EQ(derived->ref_count(), 1u);
    EXPECT_EQ(derived->type(), "Derived");

    // Implicit conversion to RefPtr<BaseClass>
    RefPtr<BaseClass> base = derived;
    EXPECT_EQ(base->ref_count(), 2u);
    EXPECT_EQ(base->type(), "Derived");

    derived.reset();
    EXPECT_EQ(base->ref_count(), 1u);
}

TEST_F(RefPtrTest, ZeroControlBlockOverhead) {
    // Intrusive reference pointer must have exactly the size of a raw pointer
    static_assert(sizeof(RefPtr<TrackedNode>) == sizeof(TrackedNode*),
                  "RefPtr must have zero secondary control block overhead");
}

TEST_F(RefPtrTest, ConcurrencyStressTest8Threads) {
    constexpr int kNumThreads = 8;
    constexpr int kIterations = 10000;

    RefPtr<TrackedNode> shared_node = make_ref<TrackedNode>(999);
    EXPECT_EQ(DestructorTracker::alive_count.load(), 1);

    std::vector<std::thread> workers;
    workers.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        workers.emplace_back([shared_node]() {
            for (int i = 0; i < kIterations; ++i) {
                // Copy assignment and reset in rapid succession
                RefPtr<TrackedNode> local1 = shared_node;
                RefPtr<TrackedNode> local2 = local1;
                ASSERT_EQ(local2->id, 999);
                local1.reset();
                RefPtr<TrackedNode> local3(std::move(local2));
                ASSERT_TRUE(local3);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    // All worker threads exited, only shared_node should remain
    EXPECT_EQ(shared_node->ref_count(), 1u);
    EXPECT_EQ(DestructorTracker::alive_count.load(), 1);

    shared_node.reset();
    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
    EXPECT_EQ(DestructorTracker::total_destroyed.load(), 1);
}

TEST_F(RefPtrTest, SharedPointerPublicationAndReclamation) {
    // Demonstrates safe cross-thread publication and reclamation via synchronized queue
    constexpr int kIterations = 5000;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::vector<RefPtr<TrackedNode>> queue;
    bool producer_done = false;

    std::thread producer([&]() {
        for (int i = 0; i < kIterations; ++i) {
            RefPtr<TrackedNode> node = make_ref<TrackedNode>(i);
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                queue.push_back(std::move(node));
            }
            queue_cv.notify_one();
        }
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            producer_done = true;
        }
        queue_cv.notify_all();
    });

    std::thread consumer([&]() {
        int consumed = 0;
        while (true) {
            RefPtr<TrackedNode> node;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_cv.wait(lock, [&]() {
                    return !queue.empty() || producer_done;
                });
                if (!queue.empty()) {
                    node = std::move(queue.back());
                    queue.pop_back();
                } else if (producer_done) {
                    break;
                }
            }
            if (node) {
                ++consumed;
                // node releases cleanly when leaving scope
            }
        }
        EXPECT_EQ(consumed, kIterations);
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(DestructorTracker::alive_count.load(), 0);
    EXPECT_EQ(DestructorTracker::total_destroyed.load(), kIterations);
}
