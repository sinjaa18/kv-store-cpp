#ifndef WAL_H
#define WAL_H

#include<string>
#include<vector>
#include "WALRecord.h"

class WAL{
    std::string filename;
    uint64_t currentSequence=0;
    uint32_t checksum(
        const std::string& key,
        const std::string& value
    );
public:
    WAL(std::string file);
    void appendPut(
        const std::string& key,
        const std::string& value
    );

    void appendDelete(
        const std::string& key
    );
};


#endif