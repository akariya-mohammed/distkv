#include "../client/client.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <thread>
#include <numeric>

using namespace distkv;

class Benchmark {
public:
    void run_all() {
        std::cout << "\n========================================\n";
        std::cout << "     DistKV Performance Benchmark\n";
        std::cout << "========================================\n\n";

        // Connect to server
        if (!client_.connect("127.0.0.1", 6379)) {
            std::cerr << "Failed to connect to server. Is it running?\n";
            std::cerr << "Start server with: ./distkv-server\n";
            return;
        }

        std::cout << "Connected to server at 127.0.0.1:6379\n\n";

        // Run benchmarks
        benchmark_set();
        benchmark_get();
        benchmark_mixed();
        benchmark_list_operations();
        benchmark_set_operations();
        benchmark_concurrent();

        std::cout << "\n========================================\n";
        std::cout << "     Benchmark Complete\n";
        std::cout << "========================================\n\n";

        client_.disconnect();
    }

private:
    Client client_;

    void benchmark_set() {
        std::cout << "Benchmarking SET operations...\n";

        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            std::string key = "bench_key_" + std::to_string(i);
            client_.set(key, "value_" + std::to_string(i));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        double ops_per_sec = (iterations * 1000.0) / duration;
        double latency_ms = static_cast<double>(duration) / iterations;

        std::cout << "  Operations: " << iterations << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n";
        std::cout << "  Avg Latency: " << std::fixed << std::setprecision(3)
                  << latency_ms << " ms\n\n";
    }

    void benchmark_get() {
        std::cout << "Benchmarking GET operations...\n";

        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            std::string key = "bench_key_" + std::to_string(i % 1000);
            client_.get(key);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        double ops_per_sec = (iterations * 1000.0) / duration;
        double latency_ms = static_cast<double>(duration) / iterations;

        std::cout << "  Operations: " << iterations << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n";
        std::cout << "  Avg Latency: " << std::fixed << std::setprecision(3)
                  << latency_ms << " ms\n\n";
    }

    void benchmark_mixed() {
        std::cout << "Benchmarking mixed operations (50% SET, 50% GET)...\n";

        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            std::string key = "mixed_key_" + std::to_string(i % 1000);

            if (i % 2 == 0) {
                client_.set(key, "value");
            } else {
                client_.get(key);
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        double ops_per_sec = (iterations * 1000.0) / duration;

        std::cout << "  Operations: " << iterations << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n\n";
    }

    void benchmark_list_operations() {
        std::cout << "Benchmarking list operations (LPUSH/RPOP)...\n";

        const int iterations = 5000;
        const std::string list_key = "bench_list";

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            client_.lpush(list_key, "item_" + std::to_string(i));
        }

        for (int i = 0; i < iterations; ++i) {
            client_.rpop(list_key);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        double ops_per_sec = (iterations * 2 * 1000.0) / duration;

        std::cout << "  Operations: " << (iterations * 2) << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n\n";
    }

    void benchmark_set_operations() {
        std::cout << "Benchmarking set operations (SADD)...\n";

        const int iterations = 5000;
        const std::string set_key = "bench_set";

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            client_.sadd(set_key, "member_" + std::to_string(i));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        double ops_per_sec = (iterations * 1000.0) / duration;

        std::cout << "  Operations: " << iterations << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n\n";

        // Cleanup
        client_.del(set_key);
    }

    void benchmark_concurrent() {
        std::cout << "Benchmarking concurrent access (4 threads)...\n";

        const int num_threads = 4;
        const int ops_per_thread = 2500;
        std::vector<std::thread> threads;
        std::vector<double> thread_times(num_threads);

        auto start = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread, &thread_times]() {
                Client client;
                client.connect("127.0.0.1", 6379);

                auto thread_start = std::chrono::high_resolution_clock::now();

                for (int i = 0; i < ops_per_thread; ++i) {
                    std::string key = "thread_" + std::to_string(t) +
                                     "_key_" + std::to_string(i);
                    client.set(key, "value");
                }

                auto thread_end = std::chrono::high_resolution_clock::now();
                thread_times[t] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    thread_end - thread_start).count();

                client.disconnect();
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        int total_ops = num_threads * ops_per_thread;
        double ops_per_sec = (total_ops * 1000.0) / duration;
        double avg_thread_time = std::accumulate(thread_times.begin(),
                                                   thread_times.end(), 0.0) /
                                 num_threads;

        std::cout << "  Threads: " << num_threads << "\n";
        std::cout << "  Total operations: " << total_ops << "\n";
        std::cout << "  Wall time: " << duration << " ms\n";
        std::cout << "  Avg thread time: " << std::fixed << std::setprecision(2)
                  << avg_thread_time << " ms\n";
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
                  << ops_per_sec << " ops/sec\n\n";
    }
};

int main() {
    Benchmark bench;
    bench.run_all();
    return 0;
}
