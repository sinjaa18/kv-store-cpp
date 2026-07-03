#ifndef WAL_H
#define WAL_H

#include<string>

class WAL{
    std::string filename;

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