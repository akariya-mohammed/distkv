#include "persistence.h"
#include <fstream>
#include <iostream>

namespace distkv {

bool Persistence::save_snapshot(const Storage& storage, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for writing: " << filepath << "\n";
        return false;
    }

    // Get snapshot from storage
    auto snapshot = storage.get_snapshot();

    // Write number of entries
    size_t count = snapshot.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write each key-value pair
    for (const auto& [key, value] : snapshot) {
        // Skip expired keys
        if (value->is_expired()) {
            continue;
        }

        // Write key
        size_t key_len = key.length();
        file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        file.write(key.c_str(), key_len);

        // Write value
        serialize_value(file, value);
    }

    std::cout << "Snapshot saved to " << filepath << " (" << count << " keys)\n";
    return true;
}

bool Persistence::load_snapshot(Storage& storage, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for reading: " << filepath << "\n";
        return false;
    }

    storage.clear();

    // Read number of entries
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    // Read each key-value pair
    for (size_t i = 0; i < count; ++i) {
        // Read key
        size_t key_len;
        file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

        std::string key(key_len, '\0');
        file.read(&key[0], key_len);

        // Read value
        auto value = deserialize_value(file);

        // Restore to storage (simplified - direct access)
        // In production, you'd use a proper restoration API
        // For now, use the public API
        if (value->type == ValueType::STRING) {
            auto str_ptr = std::static_pointer_cast<std::string>(value->data);
            storage.set(key, *str_ptr);
            if (value->expires_at != -1) {
                int ttl = static_cast<int>(value->expires_at - std::time(nullptr));
                if (ttl > 0) {
                    storage.expire(key, ttl);
                }
            }
        }
        // TODO: Handle LIST and SET types
    }

    std::cout << "Snapshot loaded from " << filepath << " (" << count << " keys)\n";
    return true;
}

bool Persistence::append_command(const std::string& filepath, const std::string& command) {
    std::ofstream file(filepath, std::ios::app);
    if (!file) {
        return false;
    }

    file << command << "\n";
    return true;
}

bool Persistence::replay_aof(Storage& storage, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        return false;
    }

    // TODO: Parse and replay commands
    // For now, this is a placeholder

    return true;
}

void Persistence::serialize_value(std::ostream& os, const std::shared_ptr<Value>& value) {
    // Write type
    uint8_t type = static_cast<uint8_t>(value->type);
    os.write(reinterpret_cast<const char*>(&type), sizeof(type));

    // Write expiry
    os.write(reinterpret_cast<const char*>(&value->expires_at), sizeof(value->expires_at));

    // Write data based on type
    switch (value->type) {
        case ValueType::STRING: {
            auto str_ptr = std::static_pointer_cast<std::string>(value->data);
            size_t len = str_ptr->length();
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(str_ptr->c_str(), len);
            break;
        }

        case ValueType::LIST: {
            auto list_ptr = std::static_pointer_cast<std::vector<std::string>>(value->data);
            size_t count = list_ptr->size();
            os.write(reinterpret_cast<const char*>(&count), sizeof(count));
            for (const auto& item : *list_ptr) {
                size_t len = item.length();
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                os.write(item.c_str(), len);
            }
            break;
        }

        case ValueType::SET: {
            auto set_ptr = std::static_pointer_cast<std::unordered_set<std::string>>(value->data);
            size_t count = set_ptr->size();
            os.write(reinterpret_cast<const char*>(&count), sizeof(count));
            for (const auto& item : *set_ptr) {
                size_t len = item.length();
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                os.write(item.c_str(), len);
            }
            break;
        }
    }
}

std::shared_ptr<Value> Persistence::deserialize_value(std::istream& is) {
    // Read type
    uint8_t type_byte;
    is.read(reinterpret_cast<char*>(&type_byte), sizeof(type_byte));
    ValueType type = static_cast<ValueType>(type_byte);

    auto value = std::make_shared<Value>(type);

    // Read expiry
    is.read(reinterpret_cast<char*>(&value->expires_at), sizeof(value->expires_at));

    // Read data based on type
    switch (type) {
        case ValueType::STRING: {
            size_t len;
            is.read(reinterpret_cast<char*>(&len), sizeof(len));
            std::string str(len, '\0');
            is.read(&str[0], len);
            value->data = std::make_shared<std::string>(str);
            break;
        }

        case ValueType::LIST: {
            size_t count;
            is.read(reinterpret_cast<char*>(&count), sizeof(count));
            auto list = std::make_shared<std::vector<std::string>>();
            for (size_t i = 0; i < count; ++i) {
                size_t len;
                is.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string item(len, '\0');
                is.read(&item[0], len);
                list->push_back(item);
            }
            value->data = list;
            break;
        }

        case ValueType::SET: {
            size_t count;
            is.read(reinterpret_cast<char*>(&count), sizeof(count));
            auto set = std::make_shared<std::unordered_set<std::string>>();
            for (size_t i = 0; i < count; ++i) {
                size_t len;
                is.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string item(len, '\0');
                is.read(&item[0], len);
                set->insert(item);
            }
            value->data = set;
            break;
        }
    }

    return value;
}

} // namespace distkv
