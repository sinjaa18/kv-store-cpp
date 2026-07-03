#include "WAL.h"
#include<fstream>

WAL::WAL(std::string file):filename(file){}


void WAL::appendPut(
const std::string& key,
const std::string& value){
    std::ofstream out(
        filename,
        std::ios::binary|
        std::ios::app
    );

    int k=key.size();
    int v=value.size();

    out.write((char*)&k,sizeof(k));
    out.write((char*)&v,sizeof(v));

    out.write(
        key.data(),
        k
    );

    out.write(
        value.data(),
        v
    );
}


void WAL::appendDelete(
const std::string& key){

    std::ofstream out(
        filename,
        std::ios::binary|
        std::ios::app
    );

    int k=key.size();
    int v=-1;

    out.write((char*)&k,sizeof(k));
    out.write((char*)&v,sizeof(v));

    out.write(
        key.data(),
        k
    );
}