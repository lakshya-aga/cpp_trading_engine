// AI generated test case generation

// bench/bench_sweep.cpp
// Per-type cost & scaling sweep: average ns/op for add/cancel/modify as book depth grows.
// Times the matcher only (events are generated pre-decoded). Median of several runs.
#include "decoder.hpp"
#include "matcher.hpp"
#include <cstdio>
#include <vector>
#include <chrono>
#include <algorithm>
#include <random>

// Generate N decoded events directly (skip the wire format; we time the matcher,
// not the decoder). Realistic add/cancel/modify mix with a live-id set so
// cancels/modifies always target real resting orders.
static std::vector<Event> gen_events(int n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> opd(0, 99);
    std::uniform_int_distribution<int> sided(0, 1);
    std::uniform_int_distribution<long long> pxd(9900, 10100);
    std::uniform_int_distribution<long long> qtyd(1, 100);

    std::vector<Event> out;
    out.reserve(n);
    std::vector<OrderId> live;
    OrderId next_id = 1;

    for (int i = 0; i < n; ++i) {
        Event e{}; e.seq = i; e.ts = (Timestamp)i * 1000;
        int r = opd(rng);
        const char* op = (r < 50) ? "add" : (r < 70) ? "cancel" : "modify";
        if (live.empty()) op = "add";

        if (op[0] == 'a') {                      // add
            OrderId id = next_id++;
            live.push_back(id);
            e.type = MsgType::Add; e.oid = id;
            e.side = sided(rng) ? Side::Sell : Side::Buy;
            e.price = pxd(rng); e.qty = qtyd(rng);
        } else if (op[0] == 'c') {               // cancel
            std::uniform_int_distribution<size_t> pick(0, live.size() - 1);
            size_t j = pick(rng); OrderId id = live[j];
            live[j] = live.back(); live.pop_back();
            e.type = MsgType::Cancel; e.oid = id;
        } else {                                 // modify
            std::uniform_int_distribution<size_t> pick(0, live.size() - 1);
            OrderId id = live[pick(rng)];
            e.type = MsgType::Modify; e.oid = id;
            e.price = pxd(rng); e.qty = qtyd(rng);
        }
        out.push_back(e);
    }
    return out;
}

using Clock = std::chrono::steady_clock;

struct TypeStat { long long ns = 0; long long count = 0; };

int main() {
    const int sizes[] = {10, 100, 1000, 5000, 10000, 50000};
    const int RUNS = 9;   // median of 9 timed runs (plus 1 untimed warmup)

    std::printf("%8s %14s %14s %14s %14s\n",
                "N", "add ns/op", "cancel ns/op", "modify ns/op", "events/sec");
    std::printf("------------------------------------------------------------------------\n");

    for (int N : sizes) {
        auto events = gen_events(N, 12345);

        std::vector<double> add_runs, cxl_runs, mod_runs, total_runs;

        for (int run = 0; run < RUNS + 1; ++run) {   // run 0 = warmup, discarded
            OrderBook ob; Matcher m(ob);
            TypeStat add, cxl, mod;

            for (const auto& e : events) {
                auto t0 = Clock::now();
                m.apply(e);
                auto t1 = Clock::now();
                long long dt = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
                switch (e.type) {
                    case MsgType::Add:    add.ns += dt; add.count++; break;
                    case MsgType::Cancel: cxl.ns += dt; cxl.count++; break;
                    case MsgType::Modify: mod.ns += dt; mod.count++; break;
                    default: break;
                }
            }
            if (run == 0) continue;

            add_runs.push_back(add.count ? (double)add.ns / add.count : 0);
            cxl_runs.push_back(cxl.count ? (double)cxl.ns / cxl.count : 0);
            mod_runs.push_back(mod.count ? (double)mod.ns / mod.count : 0);
            total_runs.push_back((double)(add.ns + cxl.ns + mod.ns));
        }

        auto med = [](std::vector<double>& v){ std::sort(v.begin(), v.end()); return v[v.size() / 2]; };
        double add_ns = med(add_runs), cxl_ns = med(cxl_runs), mod_ns = med(mod_runs);
        double tot_ns = med(total_runs);
        double eps = tot_ns > 0 ? (N / (tot_ns / 1e9)) : 0;

        std::printf("%8d %14.1f %14.1f %14.1f %14.0f%s\n",
                    N, add_ns, cxl_ns, mod_ns, eps,
                    N < 1000 ? "  (noisy)" : "");
    }
    return 0;
}