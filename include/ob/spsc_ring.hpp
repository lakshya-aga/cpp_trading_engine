#pragma once
#include <atomic>
#include <array>
#include <cstddef>
template <typename T, size_t capacity>
class Spsc{
    public:
        // template<typename T, size_t capacity>
        Spsc(): head{0}, tail{0} {}

        // template <typename T, size_t capacity>
        bool push(const T& entry) noexcept{
            auto h_ = head.load();
            if(capacity == h_-tail.load(std::memory_order_acquire)) return false;
            queue[h_%(capacity)]=entry;
            head.store(h_+1, std::memory_order_release);
            return true;
        }
        // template <typename T, size_t capacity>
        bool pop(T& item) noexcept{
            auto t_=tail.load();
            if(0 == head.load(std::memory_order_acquire) - t_) return false;
            item = std::move(queue[t_%(capacity)]);
            tail.store(1+t_, std::memory_order_release);
            return true;
        }

    private:
        std::array<T, capacity> queue;
        std::atomic<size_t> head;
        std::atomic<size_t> tail;
        
};