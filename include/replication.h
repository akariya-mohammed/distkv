#ifndef DISTKV_REPLICATION_H
#define DISTKV_REPLICATION_H

#include <string>
#include <vector>
#include <atomic>

namespace distkv {

// Placeholder for replication functionality
// Will be implemented in future phases

class ReplicationMaster {
public:
    void register_slave(int fd);
    void replicate_command(const std::string& cmd);

private:
    std::vector<int> slave_fds_;
};

class ReplicationSlave {
public:
    void connect_to_master(const std::string& host, int port);
    void sync_from_master();

private:
    int master_fd_;
};

} // namespace distkv

#endif // DISTKV_REPLICATION_H
