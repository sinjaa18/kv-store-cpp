#ifndef WALRECORD_H
#define WALRECORD_H

#include <cstdint>

enum Operation:uint8_t{
    PUT=1,
    DELETE=2
};

struct WALRecord{
    uint64_t sequence;
    Operation op;
    uint32_t keySize;
    uint32_t valueSize;
    uint32_t checksum;

};

#endif