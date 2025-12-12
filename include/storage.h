#ifndef DISTKV_STORAGE_H
#define DISTKV_STORAGE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <optional>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <ctime>

namespace distkv {

// Supported value types
enum class ValueType {
    STRING,
    LIST,
    SET
};

// Value wrapper for different types
struct Value {
    ValueType type;
    std::shared_ptr<void> data;
    time_t expires_at;  // -1 for no expiry

    Value(ValueType t) : type(t), expires_at(-1) {}

    // Helper to check if expired
    bool is_expired() const {
        if (expires_at == -1) return false;
        return std::time(nullptr) > expires_at;
    }
};

// Main storage engine
class Storage {
public:
    Storage();
    ~Storage();

    // String operations
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);

    // Generic operations
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int seconds);
    int ttl(const std::string& key);

    // List operations
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    std::optional<std::vector<std::string>> lrange(const std::string& key, int start, int stop);
    int llen(const std::string& key);

    // Set operations
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    std::optional<std::unordered_set<std::string>> smembers(const std::string& key);
    int scard(const std::string& key);

    // Utility
    size_t dbsize() const;
    void clear();
    std::vector<std::string> keys() const;

    // For persistence
    std::unordered_map<std::string, std::shared_ptr<Value>> get_snapshot() const;
    void restore_snapshot(const std::unordered_map<std::string, std::shared_ptr<Value>>& data);

private:
    std::unordered_map<std::string, std::shared_ptr<Value>> data_;
    mutable std::shared_mutex mutex_;  // Reader-writer lock

    // Helper to clean up expired keys
    void cleanup_expired(const std::string& key);

    // Type checking helpers
    bool check_type(const std::string& key, ValueType expected_type);
    std::shared_ptr<Value> get_or_create(const std::string& key, ValueType type);
};

} // namespace distkv

#endif // DISTKV_STORAGE_H
