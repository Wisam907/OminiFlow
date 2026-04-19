#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include "lock_free_queue.hpp"
#include "packet_handler.hpp"

int main() {
    using namespace ominiflow;

    constexpr size_t TOTAL_PACKETS = 1'000'000;   // 压测数量：100万
    LockFreeQueue<std::unique_ptr<DataPacket>, 1024> queue;
    
    std::atomic<bool> running{true};
    std::atomic<size_t> total_processed{0};       // 仅用于最终统计

    // 消费者线程
    std::thread consumer([&]() {
        std::unique_ptr<DataPacket> packet;
        size_t local_count = 0;                   // 本地计数，避免原子操作

        // 阶段1：running == true 时持续处理
        while (running) {
            if (queue.pop(std::move(packet))) {
                ++local_count;
                // 可选：极低频率打印（压测时建议注释）
                // if (local_count % 100'000 == 0) {
                //     std::cout << "Progress: " << local_count << "/" << TOTAL_PACKETS << "\n";
                // }
                packet.reset();
            } else {
                // 队列空且 running 可能即将变为 false，让出 CPU 避免空转
                std::this_thread::yield();
            }
        }

        // 阶段2：running 已为 false，处理队列中剩余元素直到为空
        while (queue.pop(std::move(packet))) {
            ++local_count;
            packet.reset();
        }

        total_processed = local_count;            // 一次性更新全局计数
    });

    // 生产者线程
    std::thread producer([&]() {
        for (size_t i = 0; i < TOTAL_PACKETS; ++i) {
            std::vector<uint8_t> fake_raw(64, 0);
            auto packet = std::make_unique<DataPacket>(std::move(fake_raw));
            while (!queue.push(std::move(packet))) {
                std::this_thread::yield();
            }
        }
        std::cout << "Producer finished after sending " << TOTAL_PACKETS << " packets.\n";
    });

    auto start = std::chrono::steady_clock::now();

    producer.join();

    // 通知消费者停止接收新任务（无需 sleep）
    running = false;
    consumer.join();

    auto end = std::chrono::steady_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double throughput = total_processed.load() / (elapsed_ms / 1000.0);

    std::cout << "Consumer processed " << total_processed << " packets.\n";
    std::cout << "Time: " << elapsed_ms << " ms\n";
    std::cout << "Throughput: " << throughput << " pkt/s\n";

    return 0;
}