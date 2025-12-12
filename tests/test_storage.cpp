#include "../include/storage.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

using namespace distkv;

// Test fixture
class TestRunner {
public:
    void run_all() {
        test_string_operations();
        test_list_operations();
        test_set_operations();
        test_expiration();
        test_concurrent_access();

        std::cout << "\n=================================\n";
        std::cout << "All tests passed! ✓\n";
        std::cout << "=================================\n";
    }

private:
    void test_string_operations() {
        std::cout << "Testing string operations... ";
        Storage storage;

        // SET and GET
        assert(storage.set("key1", "value1"));
        auto val = storage.get("key1");
        assert(val.has_value());
        assert(*val == "value1");

        // Update value
        assert(storage.set("key1", "value2"));
        val = storage.get("key1");
        assert(*val == "value2");

        // GET non-existent key
        val = storage.get("nonexistent");
        assert(!val.has_value());

        // DELETE
        assert(storage.del("key1"));
        assert(!storage.exists("key1"));

        // DELETE non-existent
        assert(!storage.del("nonexistent"));

        std::cout << "✓\n";
    }

    void test_list_operations() {
        std::cout << "Testing list operations... ";
        Storage storage;

        // LPUSH
        assert(storage.lpush("mylist", "item1"));
        assert(storage.lpush("mylist", "item2"));
        assert(storage.llen("mylist") == 2);

        // RPUSH
        assert(storage.rpush("mylist", "item3"));
        assert(storage.llen("mylist") == 3);

        // LPOP
        auto item = storage.lpop("mylist");
        assert(item.has_value());
        assert(*item == "item2");
        assert(storage.llen("mylist") == 2);

        // RPOP
        item = storage.rpop("mylist");
        assert(item.has_value());
        assert(*item == "item3");

        // LRANGE
        storage.rpush("mylist", "a");
        storage.rpush("mylist", "b");
        storage.rpush("mylist", "c");

        auto range = storage.lrange("mylist", 0, -1);
        assert(range.has_value());
        assert(range->size() == 4);
        assert((*range)[0] == "item1");
        assert((*range)[3] == "c");

        // Range with indices
        range = storage.lrange("mylist", 1, 2);
        assert(range->size() == 2);
        assert((*range)[0] == "a");
        assert((*range)[1] == "b");

        std::cout << "✓\n";
    }

    void test_set_operations() {
        std::cout << "Testing set operations... ";
        Storage storage;

        // SADD
        assert(storage.sadd("myset", "member1"));
        assert(storage.sadd("myset", "member2"));
        assert(!storage.sadd("myset", "member1")); // Duplicate
        assert(storage.scard("myset") == 2);

        // SISMEMBER
        assert(storage.sismember("myset", "member1"));
        assert(!storage.sismember("myset", "nonexistent"));

        // SREM
        assert(storage.srem("myset", "member1"));
        assert(!storage.sismember("myset", "member1"));
        assert(storage.scard("myset") == 1);

        // SMEMBERS
        storage.sadd("myset", "a");
        storage.sadd("myset", "b");
        auto members = storage.smembers("myset");
        assert(members.has_value());
        assert(members->size() == 3);

        std::cout << "✓\n";
    }

    void test_expiration() {
        std::cout << "Testing expiration... ";
        Storage storage;

        // Set key with expiration
        storage.set("tempkey", "tempvalue");
        assert(storage.expire("tempkey", 2));

        // Check TTL
        int ttl = storage.ttl("tempkey");
        assert(ttl > 0 && ttl <= 2);

        // Key should exist
        assert(storage.exists("tempkey"));

        // Wait for expiration
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Key should be expired
        assert(!storage.exists("tempkey"));
        auto val = storage.get("tempkey");
        assert(!val.has_value());

        std::cout << "✓\n";
    }

    void test_concurrent_access() {
        std::cout << "Testing concurrent access... ";
        Storage storage;

        const int num_threads = 10;
        const int ops_per_thread = 1000;

        std::vector<std::thread> threads;

        // Concurrent writes
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&storage, i, ops_per_thread]() {
                for (int j = 0; j < ops_per_thread; ++j) {
                    std::string key = "thread" + std::to_string(i) +
                                     "_key" + std::to_string(j);
                    storage.set(key, "value");
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Verify all keys exist
        assert(storage.dbsize() == num_threads * ops_per_thread);

        threads.clear();

        // Concurrent reads
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&storage, i, ops_per_thread]() {
                for (int j = 0; j < ops_per_thread; ++j) {
                    std::string key = "thread" + std::to_string(i) +
                                     "_key" + std::to_string(j);
                    auto val = storage.get(key);
                    assert(val.has_value());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::cout << "✓\n";
    }
};

int main() {
    std::cout << "\n=================================\n";
    std::cout << "Running DistKV Storage Tests\n";
    std::cout << "=================================\n\n";

    TestRunner runner;
    runner.run_all();

    return 0;
}
