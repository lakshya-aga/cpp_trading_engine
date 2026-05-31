// AI generated Run file for testing feed generator and consumer
#include "decoder.hpp"
#include <fstream>
#include <vector>
#include <cstdio>
#include <iostream>

int main() {
    std::ifstream f("feed.bin", std::ios::binary | std::ios::ate);
    std::streamsize sz = f.tellg();
    if (sz <= 0){
        std::cout<<"Overflow detected"<<std::endl;
        return -1;
    }
    f.seekg(0);
    std::vector<std::byte> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), sz);

    Decoder d(buf);
    Event e;
    int n = 0;
    while (d.next(e)) n++;
    printf("decoded %d records\n", n);
}