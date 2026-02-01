#pragma once
#include <array>
#include <atomic>
#include <type_traits>
#include <cassert>

namespace hft::core {

/**
 * @brief SPSC Ring Buffer
 *
 * @tparam T
 * @tparam Capacity
 */
template <typename T, std::size_t Capacity>
class SPSCRingBuffer {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");
    static_assert((Capacity != 0u) && ((Capacity & (Capacity - 1)) == 0), "Capacity must be power of 2.");

    static constexpr std::size_t Mask = Capacity - 1;

public:
    /**
     * @brief Producer side responsible from Push
     *
     * @param value
     * @return true
     * @return false
     */
    [[nodiscard]] auto Push(const T& value) noexcept -> bool
    {
        const std::size_t curr_tail = tail.load(std::memory_order_relaxed);
        const std::size_t next_tail = (curr_tail + 1) & Mask;

        // We look at head (owned by Consumer) to see if there is room
        if (next_tail == head.load(std::memory_order_acquire)) [[unlikely]] {
            drop_count.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        buffer_[curr_tail] = value;
        // Signals to Consumer that data is ready
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Consumer size responsible from pop
     *
     * @param out_value
     * @return true
     * @return false
     */
    [[nodiscard]] auto Pop(T& out_value) noexcept -> bool
    {
        const std::size_t curr_head = head.load(std::memory_order_relaxed);

        // We look at tail (owned by Producer) to see if there is data
        if (curr_head == tail.load(std::memory_order_acquire)) {
            return false;
        }

        out_value = buffer_[curr_head];
        // Signals to Producer that a slot is now free
        head.store((curr_head + 1) & Mask, std::memory_order_release);
        return true;
    }

    /**
     * @brief Get the Drop Count object
     *
     * @return std::size_t
     */
    [[nodiscard]] auto GetDropCount() const noexcept -> std::size_t
    {
        return drop_count.load(std::memory_order_relaxed);
    }

    /**
     * @brief Is Empty check
     *
     * @return true
     * @return false
     */
    [[nodiscard]] auto Empty() const noexcept -> bool
    {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }

private:
    // Aligning the buffer and indices to separate cache lines (64 bytes)
    // prevents "False Sharing" which would destroy HFT performance.
    alignas(64) std::array<T, Capacity> buffer_ {};

    alignas(64) std::atomic<std::size_t> tail { 0 }; // Producer controlled
    alignas(64) std::atomic<std::size_t> head { 0 }; // Consumer controlled

    alignas(64) std::atomic<std::size_t> drop_count { 0 };
};

} // namespace hft::core
