#include "WAL.h"
#include<fstream>

WAL::WAL(std::string file)
:filename(file){}

uint32_t WAL::checksum(
const std::string& key,
const std::string& value){
    uint32_t hash=0;
    for(char c:key)
        hash=hash*31+c;
    for(char c:value)
        hash=hash*31+c;
    return hash;
}

void WAL::appendPut(
const std::string& key,
const std::string& value){
    std::ofstream out(
        filename,
        std::ios::binary|
        std::ios::app
    );


    WALRecord rec;
    rec.sequence=++currentSequence;
    rec.op=PUT;
    rec.keySize=key.size();
    rec.valueSize=value.size();
    rec.checksum=
    checksum(key,value);

    out.write(
    (char*)&rec,
    sizeof(rec));

    out.write(
    key.data(),
    key.size());

    out.write(
    value.data(),
    value.size());
}

void WAL::appendDelete(
const std::string& key){
    std::ofstream out(
        filename,
        std::ios::binary|
        std::ios::app
    );

    WALRecord rec;

    rec.sequence=++currentSequence;
    rec.op=DELETE;
    rec.keySize=key.size();
    rec.valueSize=0;
    rec.checksum=
    checksum(key,"");

    out.write(
    (char*)&rec,
    sizeof(rec));

    out.write(
    key.data(),
    key.size());
}