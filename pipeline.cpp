#include "spsc_ring.hpp"
#include "order_book.hpp"
#include "decoder.hpp"
#include "matcher.hpp"
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>


inline void cpu_relax() {
  #if defined(__x86_64__) || defined(_M_X64)
      _mm_pause();
  #elif defined(__aarch64__) || defined(__arm__)
      __asm__ __volatile__("yield" ::: "memory");
  #else
      ; // nothing
  #endif
  }


using namespace std;

std::vector<std::byte> buf;

std::atomic<bool> done{false};
long long add_events(Decoder d, Spsc<Event, 4096>& ring){
    long long events = 0;
    Event e;
    while(d.next(e)){
        events++;
        ring.push(e);
    }
    done.store(true, std::memory_order_release);
    cout<<"Events matching completed:"<<events<<"\n";
    return events;
}

long long match_events(Matcher m, Spsc<Event, 4096>& ring){
    Event temp;
    long long trades = 0;
    while(true){   
        if(ring.pop(temp)) trades += m.apply(temp).size();
        else if(done.load(std::memory_order_acquire)) break;
        else cpu_relax();
    }
    cout<<"Trades matching completed:"<<trades<<"\n";
    return trades;
}

int main(int argc, char** argv){
    Spsc<Event, 4096> ring;

    std::ifstream f("feed.bin", std::ios::binary | std::ios::ate);

    if (!f) { std::cout<<"cannot open\n"; return 1; }

    auto sz = f.tellg(); // current pointer
    if (sz <= 0) { std::cout<<"bad size\n"; return 1; }
    f.seekg(0);
    buf = std::vector<std::byte>(sz);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    Decoder d = Decoder(buf);
    OrderBook ob;
    Matcher m(ob);

    std::thread producer(add_events, std::move(d), std::ref(ring));
    std::thread consumer(match_events, std::move(m), std::ref(ring));

    producer.join();
    consumer.join();
}