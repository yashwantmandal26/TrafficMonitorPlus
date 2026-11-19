// TrafficMonitorPlus - Thread-Safe Fixed-Size Ring Buffer
#pragma once
#include "../pch.h"

namespace tmp {

// Fixed-capacity ring buffer suitable for sparkline data.
// T must be default-constructible and copyable.
// N is the compile-time maximum number of elements.
// All public methods are thread-safe.
template <typename T, size_t N>
class RingBuffer {
    static_assert(N > 0, "RingBuffer capacity must be greater than 0");

public:
    RingBuffer() noexcept : head_(0), count_(0), data_{} {}

    // Add a value. Overwrites the oldest entry if the buffer is full.
    void push(T value) {
        std::lock_guard lock(mutex_);
        data_[head_] = std::move(value);
        head_ = (head_ + 1) % N;
        if (count_ < N) ++count_;
    }

    // Number of elements currently stored.
    [[nodiscard]] size_t size() const {
        std::lock_guard lock(mutex_);
        return count_;
    }

    // Maximum number of elements.
    [[nodiscard]] static constexpr size_t capacity() noexcept { return N; }

    // Access element by logical index (0 = oldest, count-1 = newest).
    // Caller must ensure i < size(). Not bounds-checked for performance.
    [[nodiscard]] T operator[](size_t i) const {
        std::lock_guard lock(mutex_);
        size_t real = ((head_ + N - count_) + i) % N;
        return data_[real];
    }

    // Most recently pushed value. Undefined if empty.
    [[nodiscard]] T back() const {
        std::lock_guard lock(mutex_);
        size_t idx = (head_ + N - 1) % N;
        return data_[idx];
    }

    // Remove all elements.
    void clear() {
        std::lock_guard lock(mutex_);
        head_ = 0;
        count_ = 0;
        data_ = {};
    }

    // ─── Snapshot-based iteration ────────────────────────────
    // Because the buffer is mutex-protected, we expose iteration through
    // a Snapshot: a linearized, immutable copy of the current contents.
    //
    // Usage:
    //   auto snap = ring.snapshot();
    //   for (auto v : snap) { ... }
    //
    // Or directly (creates a temporary Snapshot via begin()/end()):
    //   for (auto v : ring.snapshot()) { ... }

    class Snapshot {
    public:
        using iterator = typename std::vector<T>::const_iterator;

        explicit Snapshot(const RingBuffer& rb) {
            std::lock_guard lock(rb.mutex_);
            data_.reserve(rb.count_);
            for (size_t i = 0; i < rb.count_; ++i) {
                size_t real = ((rb.head_ + N - rb.count_) + i) % N;
                data_.push_back(rb.data_[real]);
            }
        }

        [[nodiscard]] iterator begin() const noexcept { return data_.cbegin(); }
        [[nodiscard]] iterator end()   const noexcept { return data_.cend(); }
        [[nodiscard]] size_t   size()  const noexcept { return data_.size(); }
        [[nodiscard]] bool     empty() const noexcept { return data_.empty(); }
        [[nodiscard]] const T& operator[](size_t index) const { return data_[index]; }

    private:
        std::vector<T> data_;
    };

    // Create a snapshot for safe iteration.
    [[nodiscard]] Snapshot snapshot() const { return Snapshot(*this); }

    // begin()/end() — creates a linearized copy for range-for.
    // IMPORTANT: Callers should use `snapshot()` and iterate the returned
    // Snapshot object. Direct `for (auto v : ring)` would call begin() and
    // end() independently, producing iterators from different snapshots.
    // Use: `for (auto v : ring.snapshot()) { ... }`
    //
    // These are provided for compatibility with algorithms that accept
    // a begin/end pair from the same call site:
    //   auto s = ring.snapshot();
    //   std::for_each(s.begin(), s.end(), ...);

private:
    mutable std::mutex mutex_;
    std::array<T, N>   data_;
    size_t             head_;   // Next write position
    size_t             count_;  // Current number of elements
};

} // namespace tmp
