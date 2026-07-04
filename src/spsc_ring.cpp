#pragma once
#include <atomic>
#include <array>
#include <cstddef>
template <typename T, size_t capacity>
class Spsc{
    public:
        // template<typename T, size_t capacity>
        Spsc(): head{0}, tail{0}, cached_head{0}, cached_tail{0} {}

        // template <typename T, size_t capacity>
        bool push(const T& entry) noexcept{
            auto h_ = head.load(std::memory_order_relaxed);
            if (h_ - cached_tail == capacity) {                   // cache says full?
                    cached_tail = tail.load(std::memory_order_acquire); // refresh from real tail
                    if (h_ - cached_tail == capacity) return false;   // really full
            }       
            queue[h_%(capacity)]=entry;
            head.store(h_+1, std::memory_order_release);
            return true;
        }
        // template <typename T, size_t capacity>
        bool pop(T& item) noexcept{
            auto t_=tail.load(std::memory_order_relaxed);
            if(0 == cached_head - t_) {
                cached_head = head.load(std::memory_order_acquire);
                if(cached_head-t_==0)
                return false;
            }
            item = std::move(queue[t_%(capacity)]);
            tail.store(1+t_, std::memory_order_release);
            return true;
        }

    private:
        std::array<T, capacity> queue;
        alignas(64) std::atomic<size_t> head;
        size_t cached_tail;
        
        alignas(64) std::atomic<size_t> tail;
        size_t cached_head;
        
};