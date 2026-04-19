#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
namespace ominiflow {

#ifdef __cpp_lib_hardware_interference_size
    // 如果标准库支持，则优先使用标准定义
    constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
#elif defined(__APPLE__) && defined(__aarch64__)
    // Apple Silicon (M1/M2/M3) 使用 128 字节缓存行
    constexpr std::size_t CACHE_LINE_SIZE = 128;
#elif defined(__x86_64__) || defined(__i386__)
    // x86/x86-64 架构，通常是 64 字节
    constexpr std::size_t CACHE_LINE_SIZE = 64;
#elif defined(__arm__) || defined(__aarch64__)
    // ARM 架构保守估计为 64 字节
    constexpr std::size_t CACHE_LINE_SIZE = 64;
#else
    // 未知平台，使用一个安全的默认值 64 字节
    constexpr std::size_t CACHE_LINE_SIZE = 64;
#endif

/**
 * @brief 环形无锁队列（MPMC）
 * @tparam T 存储的数据类型
 * @tparam Size 队列大小，必须是2的次幂以优化取模运算
 * 
 */
template<typename T, size_t Size = 1024>
class LockFreeQueue {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

public:
    LockFreeQueue() : head_(0), tail_(0) {
        for (size_t i = 0; i < Size; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    // 生产者入队
    bool push(T&& data) {
        Cell *cell;
        size_t pos = tail_.load(std::memory_order_relaxed);

        while (true) {
            cell = &buffer_[pos & (Size - 1)];
            size_t seq = cell ->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                // 试图占有这个位置
                if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // 队列已满
                return false;
            } else {
                // 重新读取位置
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        cell->data = std::move(data);
        // 增加序列号，通知消费者可以读取了
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    // 消费者出队
    bool pop(T&& data) {
        Cell *cell;
        size_t pos = head_.load(std::memory_order_relaxed);

        while (true) {
            cell = &buffer_[pos & (Size - 1)];
            size_t seq = cell ->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

            if (diff == 0) {
                // 试图占有这个位置进行读取
                if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // 队列为空
                return false;
            } else {
                pos = head_.load(std::memory_order_relaxed);
            }
        }

        data = std::move(cell->data);
        // 更新序列号，标记此位置为空，准备给下一轮生产者使用
        cell->sequence.store(pos + Size, std::memory_order_release);
        return true;
    }

private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };

    // 使用alignas避免伪共享导致的性能陷阱
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_;
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_;

    Cell buffer_[Size];
};
}