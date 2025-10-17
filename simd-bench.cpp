#include <benchmark/benchmark.h>
#include <algorithm>
#include <vector>
#include <immintrin.h>
#include <numeric>
#include <execution>
#include <random>
#include <iostream>

int64_t findMax_std(std::vector<int64_t>& arr)
{
    return *max_element(std::execution::unseq, arr.begin(), arr.end());
}

__attribute__((target("avx512f,avx512vl")))
int64_t findMax_intrinsics(std::vector<int64_t>& arr)
{
    // Assume: size is a multiple of 8
    const std::size_t stride = 8;
    const std::size_t arrSize = arr.size();
    const int64_t* data = arr.data();
    __m512i vmax = _mm512_loadu_si512((__m512i*)data); // first 8 elements

    for (std::size_t i = stride; i < arrSize; i += stride) {
        __m512i v = _mm512_loadu_si512((__m512i*)(data + i));
        vmax = _mm512_max_epi64(vmax, v); // pairwise max of 8 lanes
    }

    // Horizontal reduction: collapse 8 lanes into 1
    // Step 1: split high and low halves
    __m256i lo256 = _mm512_castsi512_si256(vmax);
    __m256i hi256 = _mm512_extracti64x4_epi64(vmax, 1);
    __m256i max256 = _mm256_max_epi64(lo256, hi256);

    // Step 2: reduce 256-bit -> 128-bit
    __m128i lo128 = _mm256_castsi256_si128(max256);
    __m128i hi128 = _mm256_extracti128_si256(max256, 1);
    __m128i max128 = _mm_max_epi64(lo128, hi128);

    // Step 3: reduce 128-bit -> 64-bit
    __m128i hi64 = _mm_unpackhi_epi64(max128, max128);
    __m128i max64 = _mm_max_epi64(max128, hi64);

    return _mm_cvtsi128_si64(max64);
}

inline __attribute__((always_inline))
int64_t findMaxShared(std::vector<int64_t>& arr)
{
    int64_t max = arr[0];

    for (int64_t v : arr) {
        if (max < v)
            max = v;
    }
    return max;
}

__attribute__((target("avx512f")))
int64_t findMaxAvx512(std::vector<int64_t>& arr)
{
    return findMaxShared(arr);
}

__attribute__((target("avx2")))
int64_t findMaxAvx2(std::vector<int64_t>& arr)
{
    return findMaxShared(arr);
}

__attribute__((target("arch=x86-64,no-sse,no-avx")))
int64_t findMaxScalar(std::vector<int64_t>& arr)
{
    int64_t* pArr = arr.data();
    size_t arrSize = arr.size();
    int64_t max = pArr[0];

    for (size_t i = 0; i < arrSize; ++i) {
        if (max < pArr[i])
            max = pArr[i];
    }
    return max;
}

constexpr int dataSize = 1024;

std::vector<int64_t> genData(size_t size)
{
    std::cout << "generating data...\n";
    std::vector<int64_t> d;
    d.resize(size);
    std::iota(d.begin(), d.end(), (size / 2) * -1);
    std::shuffle(d.begin(), d.end(), std::mt19937{std::random_device{}()});
    std::cout << "data vector size: " << d.size() << "\n";
    std::cout << "data elements: " << d[0] << ", " << d[1] << ", "
        << d[2] << ", ..., " << d[dataSize - 1] << "\n";
    return d;
}

std::vector<int64_t> data = genData(dataSize);

static void BM_findMax_std(benchmark::State& state)
{
    std::cout << "findMax_std(data) = " << findMax_std(data) << "\n";
    for (auto _ : state) {
        benchmark::DoNotOptimize(findMax_std(data));
    }
}
BENCHMARK(BM_findMax_std);

static void BM_findMax_intrinsics(benchmark::State& state)
{
    std::cout << "findMax_intrinsics(data) = {}" << findMax_intrinsics(data) << "\n";
    for (auto _ : state) {
        benchmark::DoNotOptimize(findMax_intrinsics(data));
    }
}
BENCHMARK(BM_findMax_intrinsics);

static void BM_findMaxAvx512(benchmark::State& state)
{
    std::cout << "findMaxAvx512(data) = " << findMaxAvx512(data) << "\n";
    for (auto _ : state) {
        benchmark::DoNotOptimize(findMaxAvx512(data));
    }
}
BENCHMARK(BM_findMaxAvx512);

static void BM_findMaxAvx2(benchmark::State& state)
{
    std::cout << "findMaxAvx2(data) = " << findMaxAvx2(data) << "\n";
    for (auto _ : state) {
        benchmark::DoNotOptimize(findMaxAvx2(data));
    }
}
BENCHMARK(BM_findMaxAvx2);

static void BM_findMaxScalar(benchmark::State& state)
{
    std::cout << "findMaxScalar(data) = " << findMaxScalar(data) << "\n";
    for (auto _ : state) {
        benchmark::DoNotOptimize(findMaxScalar(data));
    }
}
BENCHMARK(BM_findMaxScalar);

BENCHMARK_MAIN();

