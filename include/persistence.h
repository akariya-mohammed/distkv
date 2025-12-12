#ifndef DISTKV_PERSISTENCE_H
#define DISTKV_PERSISTENCE_H

#include "storage.h"
#include <string>

namespace distkv {

class Persistence {
public:
    // Save snapshot to file (RDB format)
    static bool save_snapshot(const Storage& storage, const std::string& filepath);

    // Load snapshot from file
    static bool load_snapshot(Storage& storage, const std::string& filepath);

    // Append-only file operations (AOF)
    static bool append_command(const std::string& filepath, const std::string& command);
    static bool replay_aof(Storage& storage, const std::string& filepath);

private:
    // Helper functions for serialization
    static void serialize_value(std::ostream& os, const std::shared_ptr<Value>& value);
    static std::shared_ptr<Value> deserialize_value(std::istream& is);
};

} // namespace distkv

#endif // DISTKV_PERSISTENCE_H
