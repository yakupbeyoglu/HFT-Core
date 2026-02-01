#pragma once
#include <array>
#include <atomic>
#include <type_traits>
#include <cassert>

namespace hft::core {

/**
 * @brief Overflow policy of the ring buffer
 *
 */
enum class OverflowPolicy {
    Reject, /// Reject when size == capacity
    Overwrite /// owerwrite when size == capacity
};

/**
 * @brief Ring Buffer with using of the overflow policies
 *
 * @tparam T  parameter type
 * @tparam Capacity  of the buffer
 * @tparam Policy  overflow pollicy
 */
template <typename T, std::size_t Capacity, OverflowPolicy Policy = OverflowPolicy::Reject>
class RingBuffer {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for HFT efficiency.");
    static_assert((Capacity != 0U) && ((Capacity & (Capacity - 1)) == 0), "Capacity must be a power of 2.");

    static constexpr std::size_t Mask = Capacity - 1;

public:
    /**
     * @brief Push New element
     *
     * @param value
     * @return true
     * @return false
     */
    [[nodiscard]] auto Push(const T& value) noexcept -> bool
    {
        const std::size_t current_tail = tail.load(std::memory_order_relaxed);
        const std::size_t next_tail = (current_tail + 1) & Mask;
        std::size_t current_head = head.load(std::memory_order_acquire);
        if (next_tail == current_head) [[unlikely]] {
            if constexpr (Policy == OverflowPolicy::Reject) {
                return false;
            } else {
                std::size_t next_head = (current_head + 1) & Mask;
                while (!head.compare_exchange_strong(current_head, next_head, std::memory_order_release, std::memory_order_acquire)) {
                    if (next_tail != current_head) {
                        break;
                    }
                    next_head = (current_head + 1) & Mask;
                }
            }
        }

        buffer_[current_tail] = value;
        // Release ensures the consumer sees the written data before the tail moves
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop Element from Queue
     *
     * @param out_value
     * @return true
     * @return false
     */
    [[nodiscard]] auto Pop(T& out_value) noexcept -> bool
    {
        std::size_t current_head = head.load(std::memory_order_acquire);

        while (true) {
            if (current_head == tail.load(std::memory_order_acquire)) {
                return false;
            }
            T potential_value = buffer_[current_head];
            std::size_t next_head = (current_head + 1) & Mask;
            // inm case off success its going to release, in case of fail its going to read.
            if (head.compare_exchange_strong(current_head, next_head, std::memory_order_release, std::memory_order_acquire)) {
                out_value = potential_value;
                return true;
            }
        }
    }

    /**
     * @brief Get Head element
     *
     * @return T
     */
    auto Front() const noexcept -> T
    {
        assert(!Empty());
        return buffer_[head.load(std::memory_order_relaxed)];
    }

    /**
     * @brief Empty
     *
     * @return true
     * @return false
     */
    [[nodiscard]] auto Empty() const noexcept -> bool
    {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }

    /**
     * @brief Size, Get Size of the used container.
     *
     * @return std::size_t
     */
    [[nodiscard]] auto Size() const noexcept -> std::size_t
    {
        // Correctly handles unsigned wrap-around due to Mask
        return (tail.load(std::memory_order_relaxed) - head.load(std::memory_order_relaxed)) & Mask;
    }

private:
    // Aligning to 64 bytes (typical Cache Line size) prevents "False Sharing"
    // This ensures the Producer and Consumer don't invalidate each other's caches.
    alignas(64) std::array<T, Capacity> buffer_ {};

    alignas(64) std::atomic<std::size_t> tail { 0 };
    alignas(64) std::atomic<std::size_t> head { 0 };
};

} // namespace hft::core
