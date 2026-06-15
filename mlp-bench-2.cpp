#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <immintrin.h>  // for _mm_clflush / _mm_clflushopt
#include <cstring>
#include <iostream>
#include <memory>
#include <algorithm>

#include "perf-counters.h"

class PerfCounters
{
public:
    void start()
    {
        l1PendingMisses.start();
        l1CacheLoadMisses.start();
        coreStallOnMem.start();
        lineFillBufferFull.start();
        prefetchesDropped.start();
        instRetired.start();
        branchMispredictions.start();
    }
    void stop()
    {
        l1PendingMisses.stop();
        l1CacheLoadMisses.stop();
        coreStallOnMem.stop();
        lineFillBufferFull.stop();
        prefetchesDropped.stop();
        instRetired.stop();
        branchMispredictions.stop();
    }
    void publish(benchmark::State& state)
    {
        state.counters["L1_Pending_Misses"] = benchmark::Counter(
            l1PendingMisses.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["L1_Cache_Load_Misses"] = benchmark::Counter(
            l1CacheLoadMisses.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["Core_Stall_On_Mem"] = benchmark::Counter(
            coreStallOnMem.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["Line_Fill_Buf_Full"] = benchmark::Counter(
            lineFillBufferFull.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["Prefetches_Dropped"] = benchmark::Counter(
            prefetchesDropped.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["Instructions_Run"] = benchmark::Counter(
            instRetired.read_val(),
            benchmark::Counter::kAvgThreads
        );
        state.counters["Branch_Mispredictions"] = benchmark::Counter(
            branchMispredictions.read_val(),
            benchmark::Counter::kAvgThreads
        );
    }
private:
    PerfCounter l1PendingMisses{PmuEvent::L1D_PENDING_MISSES};
    PerfCounter l1CacheLoadMisses{PmuEvent::L1D_CACHE_LOAD_MISSES};
    PerfCounter coreStallOnMem{PmuEvent::RESOURCE_STALLS_SCOREBOARD};
    PerfCounter lineFillBufferFull{PmuEvent::L1D_FB_FULL};
    PerfCounter prefetchesDropped{PmuEvent::SW_PREFETCH_T0_HITS};
    PerfCounter instRetired{PmuEvent::INSTRUCTIONS_RETIRED};
    PerfCounter branchMispredictions{PmuEvent::BRANCH_MISPREDICTIONS};
};

constexpr size_t NUM_KEYS = 10 * 1024 * 1024;

// Global shared data
static std::vector<std::unique_ptr<std::string>> keys;

// Fill memory with random data
static void init_data()
{
    std::mt19937_64 rng(12345);
    keys.resize(NUM_KEYS);
    for (auto& k : keys)
        k = std::make_unique<std::string>(rng() % 1024, 'x');

    // Shuffle to ensure randomized heap access patterns
    std::shuffle(keys.begin(), keys.end(), rng);
}

// Explicitly flush an array and its pointed string values from caches using clflush
inline void flush_from_cache(const std::vector<std::unique_ptr<std::string>>& vec)
{
    const char* data = reinterpret_cast<const char*>(vec.data());
    size_t size = vec.size() * sizeof(std::unique_ptr<std::string>);
    for (size_t i = 0; i < size; i += 64)
        _mm_clflush(data + i); // flush each cache line

    // Also flush the individual string objects and their heap buffers
    for (const auto& k : vec) {
        const char* obj_ptr = reinterpret_cast<const char*>(k.get());
        for (size_t i = 0; i < sizeof(std::string); i += 64)
            _mm_clflush(obj_ptr + i);

        const char* str_ptr = k->c_str();
        size_t len = k->length();
        for (size_t i = 0; i <= len; i += 64)
            _mm_clflush(str_ptr + i);
    }
    _mm_mfence(); // ensure flush completes before continuing
}

size_t heavyProcessing(const char* str)
{
    return strlen(str);
}

void preloadByPrefetch(const void* ptr)
{
    _mm_prefetch(ptr, _MM_HINT_T0);
}

void preloadByVolatileRead(const void* ptr)
{
    volatile char c = static_cast<const char*>(ptr)[0];
}

#define PRELOAD_BY_PREFETCH 1

#define DO_PRELOAD 1

#ifdef DO_PRELOAD

void preload(const void* ptr)
{
#if PRELOAD_BY_PREFETCH
    preloadByPrefetch(ptr);
#else
    preloadByVolatileRead(ptr);
#endif // PRELOAD_BY_PREFETCH
}

#else

#define preload(ptr) static_assert(true, "")

#endif // DO_PRELOAD


// Serial benchmark
static void BM_Serial(benchmark::State& state)
{
    int sum;
    int iterations = 0;
    PerfCounters perfCounters;

    for (auto _ : state) {
        state.PauseTiming();  // don't include flush time
        sum = 0;
        ++iterations;
        flush_from_cache(keys);
        perfCounters.start();
        state.ResumeTiming();

        for (size_t i = 0; i < keys.size(); ++i)
            sum += heavyProcessing(keys[i]->c_str());

        benchmark::DoNotOptimize(sum);
        perfCounters.stop();
    }

    perfCounters.publish(state);
}

// Batched benchmark - manually unrolled (verify template-based unrolling)
static void BM_Batched8(benchmark::State& state)
{
    constexpr size_t BATCH = 8;
    int sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    int iterations = 0;
    PerfCounters perfCounters;

    for (auto _ : state) {
        state.PauseTiming();
        flush_from_cache(keys);
        sum0 = sum1 = sum2 = sum3 = sum4 = sum5 = sum6 = sum7 = 0;
        ++iterations;
        perfCounters.start();
        state.ResumeTiming();

        for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
            preload(keys[i + 0].get());
            preload(keys[i + 1].get());
            preload(keys[i + 2].get());
            preload(keys[i + 3].get());
            preload(keys[i + 4].get());
            preload(keys[i + 5].get());
            preload(keys[i + 6].get());
            preload(keys[i + 7].get());
            preload(keys[i + 0]->c_str());
            preload(keys[i + 1]->c_str());
            preload(keys[i + 2]->c_str());
            preload(keys[i + 3]->c_str());
            preload(keys[i + 4]->c_str());
            preload(keys[i + 5]->c_str());
            preload(keys[i + 6]->c_str());
            preload(keys[i + 7]->c_str());
            sum0 += heavyProcessing(keys[i + 0]->c_str());
            sum1 += heavyProcessing(keys[i + 1]->c_str());
            sum2 += heavyProcessing(keys[i + 2]->c_str());
            sum3 += heavyProcessing(keys[i + 3]->c_str());
            sum4 += heavyProcessing(keys[i + 4]->c_str());
            sum5 += heavyProcessing(keys[i + 5]->c_str());
            sum6 += heavyProcessing(keys[i + 6]->c_str());
            sum7 += heavyProcessing(keys[i + 7]->c_str());
        }

        benchmark::DoNotOptimize(sum0);
        benchmark::DoNotOptimize(sum1);
        benchmark::DoNotOptimize(sum2);
        benchmark::DoNotOptimize(sum3);
        benchmark::DoNotOptimize(sum4);
        benchmark::DoNotOptimize(sum5);
        benchmark::DoNotOptimize(sum6);
        benchmark::DoNotOptimize(sum7);
        perfCounters.stop();
    }

    perfCounters.publish(state);
}

template<size_t IDX, size_t END>
struct Unrolled
{
    template<typename OP>
    static void loop(OP&& op)
    {
        op(IDX);
        Unrolled<IDX + 1, END>::loop(op);
    }
};

template<size_t END>
struct Unrolled<END, END>
{
    template<typename OP>
    static void loop(OP&& op) {}
};

template<uint8_t BATCH>
int calcSum_Unrolled()
{
    int sum[BATCH]{};
    for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
        Unrolled<0, BATCH>::loop([i](size_t j) {
            preload(keys[i + j].get());
            });
        Unrolled<0, BATCH>::loop([i](size_t j) {
            preload(keys[i + j]->c_str());
            });
        Unrolled<0, BATCH>::loop([&sum, i](size_t j) {
            sum[j] += heavyProcessing(keys[i + j]->c_str());
            });
    }
    int total = 0;
    Unrolled<0, BATCH>::loop([&sum, &total](size_t i) {
        total += sum[i];
        });
    return total;
}

template<uint8_t BATCH>
int calcSum()
{
    int sum[BATCH]{};
    for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
#pragma GCC unroll BATCH
        for (size_t j = 0; j < BATCH; ++j) {
            preload(keys[i + j].get());
        }
#pragma GCC unroll BATCH
        for (size_t j = 0; j < BATCH; ++j) {
            preload(keys[i + j]->c_str());
        }
#pragma GCC unroll BATCH
        for (size_t j = 0; j < BATCH; ++j) {
            sum[j] += heavyProcessing(keys[i + j]->c_str());
        }
    }
    int total = 0;
#pragma GCC unroll BATCH
    for (size_t j = 0; j < BATCH; ++j) {
        total += sum[j];
    }
    return total;
}

template<size_t BATCH>
static void BatchedGen(benchmark::State& state)
{
    int sum;
    int iterations = 0;
    PerfCounters perfCounters;

    for (auto _ : state) {
        state.PauseTiming();
        flush_from_cache(keys);
        ++iterations;
        perfCounters.start();
        state.ResumeTiming();

        sum = calcSum<BATCH>();

        benchmark::DoNotOptimize(sum);
        perfCounters.stop();
    }

    perfCounters.publish(state);
}

static void BM_BatchedGen1(benchmark::State& state)
{
    BatchedGen<1>(state);
}

static void BM_BatchedGen2(benchmark::State& state)
{
    BatchedGen<2>(state);
}

static void BM_BatchedGen4(benchmark::State& state)
{
    BatchedGen<4>(state);
}

static void BM_BatchedGen8(benchmark::State& state)
{
    BatchedGen<8>(state);
}

static void BM_BatchedGen16(benchmark::State& state)
{
    BatchedGen<16>(state);
}

static void BM_BatchedGen32(benchmark::State& state)
{
    BatchedGen<32>(state);
}

static void BM_BatchedGen64(benchmark::State& state)
{
    BatchedGen<64>(state);
}

// Batched benchmark
static void BM_Batched4(benchmark::State& state)
{
    constexpr size_t BATCH = 4;
    int sum0, sum1, sum2, sum3;
    int iterations = 0;
    PerfCounters perfCounters;

    for (auto _ : state) {
        state.PauseTiming();
        flush_from_cache(keys);
        sum0 = sum1 = sum2 = sum3 = 0;
        ++iterations;
        perfCounters.start();
        state.ResumeTiming();

        for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
            preload(keys[i + 0].get());
            preload(keys[i + 1].get());
            preload(keys[i + 2].get());
            preload(keys[i + 3].get());
            preload(keys[i + 0]->c_str());
            preload(keys[i + 1]->c_str());
            preload(keys[i + 2]->c_str());
            preload(keys[i + 3]->c_str());
            sum0 += heavyProcessing(keys[i + 0]->c_str());
            sum1 += heavyProcessing(keys[i + 1]->c_str());
            sum2 += heavyProcessing(keys[i + 2]->c_str());
            sum3 += heavyProcessing(keys[i + 3]->c_str());
        }

        benchmark::DoNotOptimize(sum0);
        benchmark::DoNotOptimize(sum1);
        benchmark::DoNotOptimize(sum2);
        benchmark::DoNotOptimize(sum3);
        perfCounters.stop();
    }

    perfCounters.publish(state);
}

/*
 * MLP demo:
 * First, just show an unrolled function (wrap strlen in "int HeavyProcessing(const char*)") and compare its run time
 * to the run time of a serial function. The result should be better despite the unrolling, because there's no MLP -
 * the actual dereference of the string is done inside strlen, and at that point, there's no scope for OOOE.
 * Second, do some manual unrolling, including the unrolling of the key access. This too should have little effect,
 * because that's not the painful miss.
 * Third, explicitly access the first char, and place it in a volatile/atomic variable to make sure the access isn't
 * optimized out. This should be a major improvement, but it's hacky, and there's an extra memory write, as the load is
 * to memory and not a register.
 * Lastly, just use a direct prefetch. It's slightly faster, and it's explicit.
 *
 * Takeaways:
 * - Naive unrolling doesn't always help if the memory accesses that are the most painful can't be parallelized.
 * - We sometimes have to write ugly code in order to make the last memory access visible.
 * - When the processing isn't heavy or the compiler can reason about it easier, either the compiler or the CPU can
 *   unlock MLP without these hacks. YMMV.
 */

 // Batched benchmark
static void BM_Batched4_no_prefetch(benchmark::State& state)
{
    constexpr size_t BATCH = 4;
    int sum0, sum1, sum2, sum3;
    int iterations = 0;
    PerfCounters perfCounters;

    for (auto _ : state) {
        state.PauseTiming();
        flush_from_cache(keys);
        sum0 = sum1 = sum2 = sum3 = 0;
        ++iterations;
        perfCounters.start();
        state.ResumeTiming();

        for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
            const char* s0 = keys[i + 0]->c_str();
            const char* s1 = keys[i + 1]->c_str();
            const char* s2 = keys[i + 2]->c_str();
            const char* s3 = keys[i + 3]->c_str();

            // The issue is likely that c_str() does not touch the heap buffer immediately. 
            // It just returns the pointer stored in the std::string object.
            // When strlen(s0) is called, it blocks waiting for s0[0] to arrive from memory.
            // If the compiler hasn't issued loads for s1[0], s2[0], s3[0] BEFORE the 
            // first strlen call, we lose MLP.
            //
            // By manually fetching the first character of each string into a volatile,
            // we force the CPU to issue the cache-missing loads for all 4 heap buffers 
            // concurrently BEFORE entering the first strlen.
            volatile char v0 = s0[0];
            volatile char v1 = s1[0];
            volatile char v2 = s2[0];
            volatile char v3 = s3[0];

            sum0 += heavyProcessing(s0);
            sum1 += heavyProcessing(s1);
            sum2 += heavyProcessing(s2);
            sum3 += heavyProcessing(s3);
        }

        benchmark::DoNotOptimize(sum0);
        benchmark::DoNotOptimize(sum1);
        benchmark::DoNotOptimize(sum2);
        benchmark::DoNotOptimize(sum3);
        perfCounters.stop();
    }
    perfCounters.publish(state);
}

// removed to fix redefinition
#if 0
static constexpr size_t IterationsLimit = 32;
#define WITH_LIMIT ->Iterations(IterationsLimit)
#else
#define WITH_LIMIT
#endif

BENCHMARK(BM_Serial) WITH_LIMIT;
BENCHMARK(BM_BatchedGen1) WITH_LIMIT;
BENCHMARK(BM_BatchedGen2) WITH_LIMIT;
BENCHMARK(BM_BatchedGen4) WITH_LIMIT;
BENCHMARK(BM_BatchedGen8) WITH_LIMIT;
BENCHMARK(BM_BatchedGen16) WITH_LIMIT;
BENCHMARK(BM_BatchedGen32) WITH_LIMIT;
BENCHMARK(BM_BatchedGen64) WITH_LIMIT;
BENCHMARK(BM_Batched4) WITH_LIMIT;
BENCHMARK(BM_Batched4_no_prefetch) WITH_LIMIT;
BENCHMARK(BM_Batched8) WITH_LIMIT;

int main(int argc, char** argv)
{
#if PRELOAD_BY_PREFETCH
    std::cout << "### Using preload by prefetch ###\n";
#else
    std::cout << "### Using preload by volatile read ###\n";
#endif

#ifndef NDEBUG
    std::cerr << "!!! RUNNING A DEBUG BUILD - IGNORE THE RESULTS !!!\n";
#endif

    init_data();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
