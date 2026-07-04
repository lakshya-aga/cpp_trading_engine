// AI generated latency meaurement file

// bench/bench_latency.cpp
// Per-event latency distribution (p50/p90/p99/p99.9/p99.99) over a few million events.
// Times Matcher::apply only; events are pre-generated decoded. Warmup discarded.
//
// NOTE: uses sort-and-index percentiles (collect all latencies, sort, index).
// Correct but memory-heavy. For the final/production numbers, swap in HdrHistogram
// (brew install hdrhistogram_c, link -lhdr_histogram) and ideally rdtsc on a
// frequency-locked machine. See the marked spot below.

#include "decoder.hpp"
#include "matcher.hpp"
#include <cstdio>
#include <vector>
#include <chrono>
#include <random>

static std::vector<Event> gen_events(int n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> opd(0, 99), sided(0, 1);
    std::uniform_int_distribution<long long> pxd(9900, 10100), qtyd(1, 100);
    std::vector<Event> out; out.reserve(n);
    std::vector<OrderId> live; OrderId next_id = 1;
    for (int i = 0; i < n; ++i) {
        Event e{}; e.seq = i; e.ts = (Timestamp)i * 1000;
        int r = opd(rng); const char* op = (r < 50) ? "add" : (r < 70) ? "cancel" : "modify";
        if (live.empty()) op = "add";
        if (op[0] == 'a') {
            OrderId id = next_id++; live.push_back(id);
            e.type = MsgType::Add; e.oid = id; e.side = sided(rng) ? Side::Sell : Side::Buy;
            e.price = pxd(rng); e.qty = qtyd(rng);
        } else if (op[0] == 'c') {
            std::uniform_int_distribution<size_t> pk(0, live.size() - 1);
            size_t j = pk(rng); e.type = MsgType::Cancel; e.oid = live[j];
            live[j] = live.back(); live.pop_back();
        } else {
            std::uniform_int_distribution<size_t> pk(0, live.size() - 1);
            e.type = MsgType::Modify; e.oid = live[pk(rng)]; e.price = pxd(rng); e.qty = qtyd(rng);
        }
        out.push_back(e);
    }
    return out;
}

using Clock = std::chrono::steady_clock;

int main() {
    const int N = 2'000'000;
    const int WARMUP = 200'000;
    auto events = gen_events(N, 999);

    OrderBook ob; Matcher m(ob);
    for (int i = 0; i < WARMUP; ++i) m.apply(events[i]);

    int ops = N - WARMUP;
    auto t0 = Clock::now();
    for (int i = WARMUP; i < N; ++i) m.apply(events[i]);
    auto t1 = Clock::now();

    long long total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    std::printf("ops          : %d\n", ops);
    std::printf("total ms     : %.1f\n", total_ns / 1e6);
    std::printf("ns/op (mean) : %.1f\n", (double)total_ns / ops);
    std::printf("events/sec   : %.0f\n", ops / (total_ns / 1e9));
    return 0;
}