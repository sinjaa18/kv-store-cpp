#ifndef SSTABLEWRITER_H
#define SSTABLEWRITER_H

#include <string>
#include <fstream>
#include <vector>
#include <utility>
#include <cstdint>

class SSTableWriter {
    std::string filename;
    std::ofstream out;
    uint32_t entryCount = 0;
public:
    SSTableWriter(const std::string& file);
    ~SSTableWriter();

    bool open();
    bool append(const std::string& key, const std::string& value);
    void close();
};

#endif
