#ifndef KVPAIR_H
#define KVPAIR_H

#include <string>
#include <optional>
#include <cstdint>

struct KVPair {
    uint64_t seq;
    std::string key;
    std::optional<std::string> value;
};

#endif
