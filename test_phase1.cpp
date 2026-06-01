#include "decoder.hpp"
#include "matcher.hpp"
#include <cstdio>
#include <fstream>
#include <vector>

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "feed.bin";

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", path); return 1; }
    std::streamsize sz = f.tellg();
    if (sz <= 0) { std::fprintf(stderr, "bad size %lld\n", (long long)sz); return 1; }
    f.seekg(0);
    std::vector<std::byte> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), sz);

    OrderBook ob;
    Matcher m(ob);
    Decoder d(buf);

    Event e;
    long long events = 0, trades = 0;
    while (d.next(e)) {
        events++;
        trades += m.apply(e).size();
    }

    std::printf("events=%lld trades=%lld\n", events, trades);
    std::printf("resting bid levels=%zu ask levels=%zu open orders=%zu\n",
                ob.bids.size(), ob.asks.size(), ob.orders.size());
    return 0;
}