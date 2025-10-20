#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <immintrin.h>  // for _mm_clflush / _mm_clflushopt
#include <cstring>
#include <iostream>

constexpr size_t TABLE_SIZE = 1 * 1024 * 1024;
constexpr size_t NUM_KEYS = 64 * 1024;

// Global shared data
std::vector<std::string> table;
std::vector<size_t> keys;

// Fill memory with random data
void initData()
{
    std::mt19937_64 rng(12345);
    table.resize(TABLE_SIZE);
    for (auto& v : table)
        v = std::string(rng() % 1024, 'x');

    keys.resize(NUM_KEYS);
    for (auto& k : keys)
        k = rng() % TABLE_SIZE;
}

// Explicitly flush an array from caches using clflush
template <typename T>
inline void flushFromCache(const std::vector<T>& vec)
{
    const char* data = reinterpret_cast<const char*>(vec.data());
    size_t size = vec.size() * sizeof(T);
    for (size_t i = 0; i < size; i += 64)
        _mm_clflush(data + i); // flush each cache line
    _mm_mfence(); // ensure flush completes before continuing
}

// Serial benchmark - the baseline
static void BM_Serial(benchmark::State& state)
{
    int sum;
    int iterations = 0;

    for (auto _ : state) {
        state.PauseTiming();  // don't include flush time
        sum = 0;
        ++iterations;
        flushFromCache(table);
        // no need to flush the keys array - it's traversed sequentially anyway
        state.ResumeTiming();

        for (size_t i = 0; i < keys.size(); ++i)
            sum += strlen(table[keys[i]].c_str());

        benchmark::DoNotOptimize(sum);
    }

    std::cout << "BM_Serial final sum: " << sum << " in " << iterations << " iterations.\n";
}

// Batched benchmark - manually unrolled (verify template-based unrolling)
static void BM_Batched8(benchmark::State& state)
{
    constexpr size_t BATCH = 8;
    int sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    int iterations = 0;

    for (auto _ : state) {
        state.PauseTiming();
        flushFromCache(table);
        //flushFromCache(keys);
        sum0 = sum1 = sum2 = sum3 = sum4 = sum5 = sum6 = sum7 = 0;
        ++iterations;
        state.ResumeTiming();

        for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
            _mm_prefetch(&table[keys[i + 0]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 1]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 2]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 3]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 4]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 5]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 6]], _MM_HINT_T0);
            _mm_prefetch(&table[keys[i + 7]], _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 0]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 1]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 2]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 3]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 4]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 5]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 6]].c_str(), _MM_HINT_T0);
            _mm_prefetch(table[keys[i + 7]].c_str(), _MM_HINT_T0);
            sum0 += strlen(table[keys[i + 0]].c_str());
            sum1 += strlen(table[keys[i + 1]].c_str());
            sum2 += strlen(table[keys[i + 2]].c_str());
            sum3 += strlen(table[keys[i + 3]].c_str());
            sum4 += strlen(table[keys[i + 4]].c_str());
            sum5 += strlen(table[keys[i + 5]].c_str());
            sum6 += strlen(table[keys[i + 6]].c_str());
            sum7 += strlen(table[keys[i + 7]].c_str());
        }

        benchmark::DoNotOptimize(sum0);
        benchmark::DoNotOptimize(sum1);
        benchmark::DoNotOptimize(sum2);
        benchmark::DoNotOptimize(sum3);
        benchmark::DoNotOptimize(sum4);
        benchmark::DoNotOptimize(sum5);
        benchmark::DoNotOptimize(sum6);
        benchmark::DoNotOptimize(sum7);
    }

    int total = sum0 + sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7;
    std::cout << "BM_Batched8 final sum: " << total << " in " << iterations << " iterations.\n";
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

// This is the actual calculation work, parameterized by the size of the batch.
template<uint8_t BATCH>
int calcSum()
{
    int sum[BATCH]{};
    for (size_t i = 0; i + BATCH <= keys.size(); i += BATCH) {
        Unrolled<0, BATCH>::loop([i](size_t j) {
            _mm_prefetch(&table[keys[i + j]], _MM_HINT_T0);
            });
        Unrolled<0, BATCH>::loop([i](size_t j) {
            _mm_prefetch(table[keys[i + j]].c_str(), _MM_HINT_T0);
            });
        Unrolled<0, BATCH>::loop([&sum, i](size_t j) {
            sum[j] += strlen(table[keys[i + j]].c_str());
            });
    }
    int total = 0;
    Unrolled<0, BATCH>::loop([&sum, &total](size_t i) {
        total += sum[i];
        });
    return total;
}

template<size_t BATCH>
static void BatchedGen(benchmark::State& state)
{
    int sum;
    int iterations = 0;

    for (auto _ : state) {
        state.PauseTiming();
        flushFromCache(table);
        ++iterations;
        state.ResumeTiming();

        sum = calcSum<BATCH>();

        benchmark::DoNotOptimize(sum);
    }

    std::cout << "BM_BatchedGen" << BATCH << " final sum: " << sum << " in " << iterations << " iterations.\n";
}

// A test with batch of size 1, unrolled by the template param.
// It is expected to perform like the somewhat simpler BM_Serial function.
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

// Turn this on to set an explicit number of test iterations.
// Otherwise, the frameworks picks a number that produces accurate enough results.
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
// Validate generated benchmark: BENCHMARK(BM_Batched8) WITH_LIMIT;
BENCHMARK(BM_BatchedGen8) WITH_LIMIT;
BENCHMARK(BM_BatchedGen16) WITH_LIMIT;
BENCHMARK(BM_BatchedGen32) WITH_LIMIT;
BENCHMARK(BM_BatchedGen64) WITH_LIMIT;

int main(int argc, char** argv)
{
    initData();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}

