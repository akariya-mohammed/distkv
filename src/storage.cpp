#include "storage.h"
#include <algorithm>

namespace distkv {

Storage::Storage() {}

Storage::~Storage() {
    clear();
}

// ============= String Operations =============

bool Storage::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto val = std::make_shared<Value>(ValueType::STRING);
    val->data = std::make_shared<std::string>(value);
    data_[key] = val;

    return true;
}

std::optional<std::string> Storage::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }

    auto& val = it->second;

    // Check if expired
    if (val->is_expired()) {
        lock.unlock();
        cleanup_expired(key);
        return std::nullopt;
    }

    if (val->type != ValueType::STRING) {
        return std::nullopt;
    }

    auto str_ptr = std::static_pointer_cast<std::string>(val->data);
    return *str_ptr;
}

// ============= Generic Operations =============

bool Storage::del(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return data_.erase(key) > 0;
}

bool Storage::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    if (it->second->is_expired()) {
        lock.unlock();
        cleanup_expired(key);
        return false;
    }

    return true;
}

bool Storage::expire(const std::string& key, int seconds) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    it->second->expires_at = std::time(nullptr) + seconds;
    return true;
}

int Storage::ttl(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end()) {
        return -2;  // Key doesn't exist
    }

    auto& val = it->second;
    if (val->is_expired()) {
        return -2;
    }

    if (val->expires_at == -1) {
        return -1;  // No expiry
    }

    int remaining = static_cast<int>(val->expires_at - std::time(nullptr));
    return remaining > 0 ? remaining : -2;
}

// ============= List Operations =============

bool Storage::lpush(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto val = get_or_create(key, ValueType::LIST);
    if (!val || val->type != ValueType::LIST) {
        return false;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(val->data);
    list_ptr->insert(list_ptr->begin(), value);

    return true;
}

bool Storage::rpush(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto val = get_or_create(key, ValueType::LIST);
    if (!val || val->type != ValueType::LIST) {
        return false;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(val->data);
    list_ptr->push_back(value);

    return true;
}

std::optional<std::string> Storage::lpop(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::LIST) {
        return std::nullopt;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(it->second->data);
    if (list_ptr->empty()) {
        return std::nullopt;
    }

    std::string result = list_ptr->front();
    list_ptr->erase(list_ptr->begin());

    return result;
}

std::optional<std::string> Storage::rpop(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::LIST) {
        return std::nullopt;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(it->second->data);
    if (list_ptr->empty()) {
        return std::nullopt;
    }

    std::string result = list_ptr->back();
    list_ptr->pop_back();

    return result;
}

std::optional<std::vector<std::string>> Storage::lrange(const std::string& key, int start, int stop) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::LIST) {
        return std::nullopt;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(it->second->data);
    int size = static_cast<int>(list_ptr->size());

    // Handle negative indices
    if (start < 0) start += size;
    if (stop < 0) stop += size;

    // Clamp to valid range
    start = std::max(0, std::min(start, size - 1));
    stop = std::max(0, std::min(stop, size - 1));

    if (start > stop) {
        return std::vector<std::string>{};
    }

    std::vector<std::string> result(list_ptr->begin() + start,
                                     list_ptr->begin() + stop + 1);
    return result;
}

int Storage::llen(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::LIST) {
        return 0;
    }

    auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(it->second->data);
    return static_cast<int>(list_ptr->size());
}

// ============= Set Operations =============

bool Storage::sadd(const std::string& key, const std::string& member) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto val = get_or_create(key, ValueType::SET);
    if (!val || val->type != ValueType::SET) {
        return false;
    }

    auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(val->data);
    return set_ptr->insert(member).second;
}

bool Storage::srem(const std::string& key, const std::string& member) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::SET) {
        return false;
    }

    auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(it->second->data);
    return set_ptr->erase(member) > 0;
}

bool Storage::sismember(const std::string& key, const std::string& member) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::SET) {
        return false;
    }

    auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(it->second->data);
    return set_ptr->find(member) != set_ptr->end();
}

std::optional<std::unordered_set<std::string>> Storage::smembers(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::SET) {
        return std::nullopt;
    }

    auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(it->second->data);
    return *set_ptr;
}

int Storage::scard(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = data_.find(key);
    if (it == data_.end() || it->second->type != ValueType::SET) {
        return 0;
    }

    auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(it->second->data);
    return static_cast<int>(set_ptr->size());
}

// ============= Utility =============

size_t Storage::dbsize() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_.size();
}

void Storage::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_.clear();
}

std::vector<std::string> Storage::keys() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    std::vector<std::string> result;
    result.reserve(data_.size());

    for (const auto& [key, val] : data_) {
        if (!val->is_expired()) {
            result.push_back(key);
        }
    }

    return result;
}

// ============= Persistence Support =============

std::unordered_map<std::string, std::shared_ptr<Value>> Storage::get_snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return data_;
}

void Storage::restore_snapshot(const std::unordered_map<std::string, std::shared_ptr<Value>>& data) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    data_ = data;
}

// ============= Private Helpers =============

void Storage::cleanup_expired(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end() && it->second->is_expired()) {
        data_.erase(it);
    }
}

std::shared_ptr<Value> Storage::get_or_create(const std::string& key, ValueType type) {
    auto it = data_.find(key);

    if (it == data_.end()) {
        // Create new value
        auto val = std::make_shared<Value>(type);

        switch (type) {
            case ValueType::LIST:
                val->data = std::make_shared<std::vector<std::string>>();
                break;
            case ValueType::SET:
                val->data = std::make_shared<std::unordered_set<std::string>>();
                break;
            case ValueType::STRING:
                val->data = std::make_shared<std::string>();
                break;
        }

        data_[key] = val;
        return val;
    }

    // Check type matches
    if (it->second->type != type) {
        return nullptr;
    }

    return it->second;
}

} // namespace distkv
