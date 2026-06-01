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
#include <algorithm>
#include <random>
#include <cstdint>

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

static double pct(const std::vector<uint32_t>& sorted, double p) {
    if (sorted.empty()) return 0;
    size_t idx = (size_t)(p / 100.0 * (sorted.size() - 1));
    return sorted[idx];
}

int main() {
    const int N = 2'000'000;        // total events
    const int WARMUP = 200'000;     // untimed warmup
    auto events = gen_events(N, 999);

    OrderBook ob; Matcher m(ob);

    for (int i = 0; i < WARMUP; ++i) m.apply(events[i]);   // warmup: caches, page faults, deep book

    std::vector<uint32_t> lat; lat.reserve(N - WARMUP);
    for (int i = WARMUP; i < N; ++i) {
        // --- swap this block for HdrHistogram + rdtsc for production numbers ---
        auto t0 = Clock::now();
        m.apply(events[i]);
        auto t1 = Clock::now();
        lat.push_back((uint32_t)std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        // ----------------------------------------------------------------------
    }

    std::sort(lat.begin(), lat.end());
    double sum = 0; for (uint32_t v : lat) sum += v;

    std::printf("samples            : %zu\n", lat.size());
    std::printf("mean      (ns)     : %.1f\n", sum / lat.size());
    std::printf("min       (ns)     : %u\n", lat.front());
    std::printf("p50       (ns)     : %.0f\n", pct(lat, 50));
    std::printf("p90       (ns)     : %.0f\n", pct(lat, 90));
    std::printf("p99       (ns)     : %.0f\n", pct(lat, 99));
    std::printf("p99.9     (ns)     : %.0f\n", pct(lat, 99.9));
    std::printf("p99.99    (ns)     : %.0f\n", pct(lat, 99.99));
    std::printf("max       (ns)     : %u\n", lat.back());
    return 0;
}