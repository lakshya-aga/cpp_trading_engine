#include "spsc_ring.cpp"
#include <thread>
#include <iostream>
#include <chrono>
int main(){
    Spsc<uint64_t, 1024> queue;
    uint64_t item=0;
    long long N=1000'000'000;
    long long total = 0;
    const auto t1 = std::chrono::steady_clock::now();
    std::thread producer([&](){
        for(long long i=0; i<N; i++){
            while(!queue.push(i));
        }

    });

    std::thread consumer([&](){
        for(long long i=0; i<N; i++){
            while(!queue.pop(item));
            total+=item;
        }

    });

    producer.join();
    consumer.join();
    auto t2 = std::chrono::steady_clock::now();

    double s = std::chrono::duration<double>(t2-t1).count();
    printf("%.3f s   %.1f M items/s\n", s, N/1e6/s);    
    if(total==N*(N-1)/2)
    std::cout<<"all consumed"<<std::endl;


}

//======================= not caring about false sharing
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared      #    long long N=1'000'000;
                                                                        
// 0.082 s   12.2 M items/s
// all consumed
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared       #     long long N=100'000'000;                                                                           
// 5.739 s   17.4 M items/s
// all consumed

//======================= handling false shring using alignas


// (base) lakshya@192 src % ./thru_shared
// 5.670 s   17.6 M items/s
// all consumed
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared                                                                                  
// 5.809 s   17.2 M items/s
// all consumed
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared                                                                                  
// 0.078 s   12.8 M items/s
// all consumed
// (base) lakshya@192 src % 

// Handling false sharing after caching the head and tail for consumer and producer

// (base) lakshya@192 src % ./thru_shared                                                                                  
// 0.029 s   34.2 M items/s
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared                                                                                  
// 1.718 s   58.2 M items/s
// (base) lakshya@192 src % g++ -std=c++20 -O3 -march=native -DRING='"spsc_ring.cpp"' test_spsc.cpp -o thru_shared -pthread

// (base) lakshya@192 src % ./thru_shared                                                                                  
// 16.437 s   60.8 M items/s
// (base) lakshya@192 src % 