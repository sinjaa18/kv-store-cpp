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
    )const;

    void writeUint64(std::ofstream& out, uint64_t value);
    void writeUint32(std::ofstream& out, uint32_t value);
    void writeUint8(std::ofstream& out, uint8_t value);
public:
    WAL(std::string file);
    bool appendPut(
        const std::string& key,
        const std::string& value
    );

    bool appendDelete(
        const std::string& key
    );

    bool verifyChecksum(uint32_t expected,const std:: string&key, const std::string &value)const;

    void setCurrentSequence(uint64_t sequence);
};


#endif