Baseline Latency Measurements

**Component:** single-threaded matching engine (decode → order book → matcher)
**Status:** correctness verified (Phase 1, golden fixture under ASan). These are *unoptimized baseline* numbers — the "before" for the optimization table.

## Method

- **What is timed:** a single `Matcher::apply(Event)` call (the full path: dispatch → match/cancel/modify → book mutation → trade vector).
- **What is NOT timed:** feed generation and decoding (events are pre-decoded into a `std::vector<Event>` before measurement, so disk and parse cost never enter the loop).
- **Workload:** 2,000,000 synthetic events, ~50% add / 20% cancel / 30% modify, prices in a 200-tick band, cancels/modifies always target live orders.
- **Warmup:** first 200,000 events replayed untimed (populates caches, faults pages, builds a deep book). Measured sample = 1,800,000 events.
- **Build:** `-O3 -march=native`, GCC, C++20.
- **Timer:** `std::chrono::steady_clock` per call.

## Results (baseline, unoptimized)

| Metric | Latency (ns) |
|---|---|
| samples | 1,800,000 |
| min | 29 |
| **p50** | **256** |
| p90 | 11,904 |
| **p99** | **55,529** |
| p99.9 | 93,782 |
| p99.99 | 132,902 |
| max | 9,474,095 |
| mean | 3,798 |

(Run on a development machine without CPU-frequency locking; absolute values will shift on a quiet, frequency-pinned host. Relative structure is what matters at this stage.)

## Per-type cost & scaling (separate sweep)

Average ns/op by message type as book depth grows (median of 9 runs):

| N (events) | add ns/op | cancel ns/op | modify ns/op | events/sec |
|---|---|---|---|---|
| 1,000 | 134 | 82 | 154 | 7.45M |
| 5,000 | 154 | 86 | 202 | 6.44M |
| 10,000 | 160 | 103 | 199 | 6.30M |
| 50,000 | 156 | 133 | 229 | 5.78M |

(N < 1000 omitted — too few samples for a stable average.)

## Interpretation

1. **The p50 (256 ns) is the trustworthy "typical" number.** It reflects the common path: an `apply` that does a small amount of tree-walking and list work.

2. **The tail is enormous (p50 256 ns → p99 55µs → max 9.5ms) and is a design artifact, not matching cost.** Two suspected causes, both fixable and both expected:
   - **`apply` returns `std::vector<Trade>` by value — a heap allocation per call.** When the allocator periodically requests memory from the OS, latency spikes into the µs–ms range. This is the prime suspect for the tail and the direct justification for switching to a **sink** (pass a callback / output buffer instead of returning a vector) in optimization Phase 5.
   - **Unbounded book growth in the synthetic feed.** The random feed rests more than it crosses, so the `std::map` price-level trees grow large, increasing both per-op cost and allocator pressure.

3. **Cost grows with book depth** (add 134 → 156 ns, cancel 82 → 133 ns as N goes 1k → 50k). This is the **O(log N) `std::map`** for price levels plus the **O(depth) linear scan** inside `cancel`. A flat bucketed price array + iterator-based cancel (Phase 5) should flatten this.

4. **Modify is most expensive** (~229 ns at depth) because it is cancel + re-match — two structural operations. Consistent with the implementation.

## Caveats (read before quoting these numbers)

- **Per-call `steady_clock::now()` adds overhead** (tens of ns) that inflates the absolute p50. The relative comparisons and the tail shape are reliable; the absolute p50 is an upper bound. A production-grade measurement uses **HdrHistogram** + `rdtsc`, on a frequency-locked machine. *(This harness uses sort-and-index percentiles; swap in HdrHistogram for the final numbers.)*
- **Averages and even p99 are workload-dependent.** A feed with a realistic cross rate (tighter price band, more marketable orders) will show a different profile. Document the feed parameters with every result.

## Optimization targets identified (for Phase 5)

In priority order, justified by the data above:
1. **Eliminate per-`apply` allocation** — replace returned `std::vector<Trade>` with a sink. (Targets the tail directly.)
2. **Flatten price-level storage** — `std::map` → bounded flat array of price buckets. (Targets the O(log N) growth.)
3. **O(1) cancel** — store a `std::list::iterator` in the order index instead of `{side,price}` + linear scan. (Targets cancel's depth-dependent cost.)
4. **Re-measure after each change**, one at a time, recording a new row here.

## Next steps

- Re-run on a **quiet, frequency-locked Linux host** for stable, quotable percentiles.
- Replace sort-and-index percentiles with **HdrHistogram**.
- Run a **sampling profiler** (Instruments on macOS / perf or VTune on Linux x86) on the same replay to confirm the allocation hypothesis with a flamegraph before optimizing.