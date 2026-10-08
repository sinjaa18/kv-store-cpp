#ifndef WAL_H
#define WAL_H

#include <string>
#include <fstream>
#include <mutex>
#include <cstdint>
#include <optional>

class WAL {
    std::string filename;
    std::ofstream out;
public:
    WAL(const std::string& path);
    ~WAL();

    bool appendPut(uint64_t seq, const std::string& key, const std::string& value);
    bool appendDelete(uint64_t seq, const std::string& key);
    
    void clear();

    uint32_t calculateChecksum(const std::string& key, const std::string& value) const;
    bool verifyChecksum(uint32_t checksum, const std::string& key, const std::string& value) const;
};

#endif