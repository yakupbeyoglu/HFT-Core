#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <set>
#include "RingBuffer.hpp" // Path to your header

using namespace hft::core;

// 1. Basic Functionality Tests
TEST(RingBufferTest, BasicPushPop)
{
    RingBuffer<int, 4, OverflowPolicy::Reject> rb;

    EXPECT_TRUE(rb.Push(10));
    EXPECT_TRUE(rb.Push(20));
    EXPECT_EQ(rb.Size(), 2);

    int val;
    EXPECT_TRUE(rb.Pop(val));
    EXPECT_EQ(val, 10);
    EXPECT_TRUE(rb.Pop(val));
    EXPECT_EQ(val, 20);
    EXPECT_TRUE(rb.Empty());
}

// 2. Reject Policy Test
TEST(RingBufferTest, RejectPolicy)
{
    // Capacity 4: Can hold 3 elements (last slot is buffer between head/tail)
    RingBuffer<int, 4, OverflowPolicy::Reject> rb;

    EXPECT_TRUE(rb.Push(1));
    EXPECT_TRUE(rb.Push(2));
    EXPECT_TRUE(rb.Push(3));
    EXPECT_FALSE(rb.Push(4)); // Should fail
}

// 3. Overwrite Policy Test
TEST(RingBufferTest, OverwritePolicy)
{
    RingBuffer<int, 4, OverflowPolicy::Overwrite> rb;

    rb.Push(1);
    rb.Push(2);
    rb.Push(3);
    // Buffer is now "full" (3 elements)

    // This should move head forward and overwrite
    rb.Push(4);

    int val;
    rb.Pop(val);
    EXPECT_EQ(val, 2); // '1' should have been kicked out
    EXPECT_EQ(rb.Size(), 2);
}

// 4. Power of Two Static Assert (Manual Check)
TEST(RingBufferTest, CapacityLogic)
{
    RingBuffer<int, 16> rb;
    EXPECT_TRUE(rb.Empty());
    // Verification of size masking
    for (int i = 0; i < 15; ++i)
        rb.Push(i);
    EXPECT_EQ(rb.Size(), 15);
}

// 5. Multi-Threaded Stress Test (SPSC)
// One producer pushes 1 million items, one consumer pops them.
TEST(RingBufferTest, ConcurrencyStressTest)
{
    const int kCount = 1000000;
    RingBuffer<int, 4096, OverflowPolicy::Reject> rb;
    std::vector<int> results;
    results.reserve(kCount);

    std::thread producer([&]() {
        for (int i = 0; i < kCount; ++i) {
            while (!rb.Push(i)) {
                std::this_thread::yield(); // Wait for space
            }
        }
    });

    std::thread consumer([&]() {
        int val;
        for (int i = 0; i < kCount; ++i) {
            while (!rb.Pop(val)) {
                std::this_thread::yield(); // Wait for data
            }
            results.push_back(val);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(results.size(), kCount);
    for (int i = 0; i < kCount; ++i) {
        if (results[i] != i) {
            FAIL() << "Data mismatch at index " << i;
        }
    }
}