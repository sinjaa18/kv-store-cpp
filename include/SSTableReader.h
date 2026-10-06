#ifndef SSTABLEREADER_H
#define SSTABLEREADER_H

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include <utility>
#include <cstdint>

class SSTableReader {
    std::string filename;
    std::ifstream in;
    uint32_t entryCount = 0;
public:
    SSTableReader(const std::string& file);
    ~SSTableReader();

    bool open();
    std::optional<std::string> get(const std::string& key);
    
    std::vector<std::pair<std::string, std::string>> readAll();
    void close();
};

#endif
