## The build output

The build of the simd bench, using gcc-15 on an Intel Xeon 6527P machine:

Without `-mprefer-vector-width=512` -
```
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:215:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:29:9: optimized: basic block part vectorized using 32 byte vectors
simd-bench.cpp:29:9: optimized: basic block part vectorized using 32 byte vectors
simd-bench.cpp:123:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:215:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:257:14: optimized: loop vectorized using 32 byte vectors
mlp-bench.cpp:152:30: optimized: basic block part vectorized using 16 byte vectors
mlp-bench-2.cpp:160:30: optimized: basic block part vectorized using 16 byte vectors
```

With `-mprefer-vector-width=512` -
```
simd-bench.cpp:134:22: optimized: loop vectorized using 64 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:215:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 64 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:29:9: optimized: basic block part vectorized using 64 byte vectors
simd-bench.cpp:123:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 64 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 64 byte vectors
simd-bench.cpp:134:22: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:215:14: optimized: loop vectorized using 32 byte vectors
simd-bench.cpp:257:14: optimized: loop vectorized using 32 byte vectors
mlp-bench.cpp:152:30: optimized: basic block part vectorized using 16 byte vectors
mlp-bench-2.cpp:160:30: optimized: basic block part vectorized using 16 byte vectors
```

## SIMD bench results

### Without disabling the AVX2 preference

GCC prefers to use AVX2 instead of AVX-512 even if the latter is supported due to the CPU clock frequency downscale.
When building with `-march=native` on a modern machine, we still only get AVX2 registers. The run results:
```
2026-05-29T12:36:14+00:00
Running ./simd-bench
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.07, 0.04, 0.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_findMax_std                           591 ns          591 ns      1183725
BM_findMax_reduce                        248 ns          248 ns      2823584
BM_findMax_intrinsics                   59.5 ns         59.5 ns     11766336
BM_findMax_intrinsics_unrolled          39.8 ns         39.8 ns     17593159
BM_findMaxAvx512                         138 ns          138 ns      5068006
BM_findMaxAvx2                           139 ns          139 ns      5039922
BM_findMaxScalar                         473 ns          473 ns      1478381
BM_findMaxScalar_unrolled                244 ns          244 ns      2864199
BM_findMax_simd_portable                60.3 ns         60.3 ns     11590329
BM_findMax_simd_portable_unrolled       39.6 ns         39.6 ns     17664170
BM_findMax_simd_avx512                  59.8 ns         59.8 ns     11707283
BM_findMax_simd_avx512_unrolled         39.6 ns         39.6 ns     17666017
```

### After enabling AVX-512 using -mprefer-vector-width=512

```
2026-05-29T12:58:30+00:00
Running ./simd-bench
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.00, 0.00, 0.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_findMax_std                           594 ns          594 ns      1178790
BM_findMax_reduce                        154 ns          154 ns      4555765
BM_findMax_intrinsics                   59.4 ns         59.3 ns     11732992
BM_findMax_intrinsics_unrolled          39.8 ns         39.8 ns     17593834
BM_findMaxAvx512                        58.6 ns         58.6 ns     11952793
BM_findMaxAvx2                          60.3 ns         60.3 ns     11691862
BM_findMaxScalar                         474 ns          474 ns      1477989
BM_findMaxScalar_unrolled                244 ns          244 ns      2863947
BM_findMax_simd_portable                61.0 ns         61.0 ns     11479667
BM_findMax_simd_portable_unrolled       39.6 ns         39.6 ns     17692230
BM_findMax_simd_avx512                  60.8 ns         60.8 ns     11510312
BM_findMax_simd_avx512_unrolled         39.6 ns         39.6 ns     17677160
```

The result of `BM_findMaxAvx2` above (and the build output) indicate it's actually using AVX2, despite the limiting target.
Apparently, `prefer-vector-width=512` conflicts with `target("avx2")` - and wins. One way to force AVX2 is using
`target("arch=x86-64-v3")`, which is an arch that doesn't support AVX-512. However, this causes conflicts between the
stl code inside the parallelized loop and the stl code outside of it. To avoid it, the code in the parallelized loop
must not include stl containers, not even a range-based for loop - just plain arrays and pointers. When the implementation
is modified accordingly, `BM_findMaxAvx2` finally uses only AVX2:
```
2026-05-29T13:46:46+00:00
Running ./avx2-bench
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.06, 0.03, 0.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
---------------------------------------------------------
Benchmark               Time             CPU   Iterations
---------------------------------------------------------
BM_findMaxAvx2        344 ns          344 ns      2031778
```
Unfortunately, I couldn't find a way to use a shared implementation with the two types of targets. The `force_inline`
shared function inherits `prefer-vector-width=512`, and thus it can't be called from a function marked as 
`arch=x86-64-v3`. This issue didn't exist in older compilers (older than gcc 10 or so), which didn't have the 
don't-use-avx-512 preference. So with older compilers, inlining a shared implementation works, but with new ones, it
doesn't. This might change if AVX-512 becomes preferable again on archs with no freq scaling issues, where there will
be no need to override the preferred width in a way that can conflict with other flags.

## MLP bench results

### 64K keys

```
2026-05-29T14:17:41+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.00, 0.00, 0.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
BM_Serial                  6681043 ns      6681002 ns           95
BM_BatchedGen1             6720180 ns      6720266 ns          106
BM_BatchedGen2             6647048 ns      6646747 ns          104
BM_BatchedGen4             6354480 ns      6354610 ns          110
BM_BatchedGen8             6009589 ns      6009723 ns          116
BM_BatchedGen16            5934623 ns      5934633 ns          119
BM_BatchedGen32            5908674 ns      5908894 ns          118
BM_BatchedGen64            6329516 ns      6329231 ns          111
BM_Batched4                6357033 ns      6357186 ns          108
BM_Batched4_no_prefetch    6391371 ns      6391432 ns          108
BM_Batched8                6053905 ns      6054014 ns          116
```

Max speedup: 13% @ 32

### 1M keys

```
2026-05-29T14:20:43+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.12, 0.13, 0.06
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
BM_Serial                143328837 ns    143328470 ns            4
BM_BatchedGen1           141404516 ns    141404366 ns            5
BM_BatchedGen2           137075364 ns    137066946 ns            5
BM_BatchedGen4           132965041 ns    132963109 ns            5
BM_BatchedGen8           124288706 ns    124284764 ns            6
BM_BatchedGen16          118295154 ns    118293483 ns            6
BM_BatchedGen32          117977057 ns    117971272 ns            6
BM_BatchedGen64          132839891 ns    132832798 ns            5
BM_Batched4              134260500 ns    134255294 ns            5
BM_Batched4_no_prefetch  133130955 ns    133118389 ns            5
BM_Batched8              124774616 ns    124774549 ns            6
```

Max speedup: 21% @ 32

### 10M keys

```
2026-05-29T14:22:20+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.46, 0.27, 0.12
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
BM_Serial               1725312718 ns   1725251343 ns            1
BM_BatchedGen1          1690715125 ns   1690643948 ns            1
BM_BatchedGen2          1560833671 ns   1560764516 ns            1
BM_BatchedGen4          1463230842 ns   1463166794 ns            1
BM_BatchedGen8          1380208347 ns   1380158432 ns            1
BM_BatchedGen16         1329694003 ns   1329614468 ns            1
BM_BatchedGen32         1323742366 ns   1323662075 ns            1
BM_BatchedGen64         1457545832 ns   1457474050 ns            1
BM_Batched4             1486813236 ns   1486744313 ns            1
BM_Batched4_no_prefetch 1497430639 ns   1497372242 ns            1
BM_Batched8             1370926383 ns   1370877963 ns            1
```

Max speedup: 30% @ 32

### Perf counters insights

```
Benchmark	Time (ns)	CPU (ns)	Iterations	Branch Mispredictions	Core Stall On Mem	Instructions Run	L1 Cache Load Misses	L1 Pending Misses	Line Fill Buf Full	Prefetches Dropped
BM_Serial	2438182378	2437087387	1	16.3591M	4.80958M	870.018M	236.596M	79.3819G	2.38388G	0
BM_BatchedGen1	1719694213	1718951449	1	16.2180M	15.2597M	953.903M	215.031M	73.2548G	3.28259G	14.6874M
BM_BatchedGen2	1577201838	1576546154	1	15.1734M	27.8839M	911.960M	214.858M	68.4755G	3.08567G	10.2584M
BM_BatchedGen4	1505707107	1505078823	1	15.1733M	10.4853M	893.610M	217.566M	70.7128G	3.51908G	13.7062M
BM_BatchedGen8	1388204158	1387497300	1	17.8207M	6.44183M	885.745M	208.579M	65.6042G	3.25599G	12.3400M
BM_BatchedGen16	1340209746	1339507207	1	16.6973M	2.94482M	885.090M	213.723M	66.3635G	3.40796G	11.5697M
BM_BatchedGen32	1352473594	1351832629	1	17.1231M	2.91511M	888.039M	219.685M	66.6658G	3.50911G	10.2244M
BM_BatchedGen64	1445288655	1444603880	1	17.7724M	9.11511M	890.660M	225.773M	67.4044G	3.64165G	5.70605M
BM_Batched4	1496302449	1495673714	1	15.7090M	20.1663M	896.231M	216.307M	68.0898G	3.27255G	16.0871M
BM_Batched4_no_prefetch	1493608736	1492804559	1	15.3353M	19.6514M	875.259M	216.376M	69.3504G	3.30651G	0
BM_Batched8	1377762888	1377038627	1	16.4404M	7.61173M	887.056M	208.715M	65.3221G	3.27725G	12.1097M
```

Findings:
* Since prefetching is speculative, it has lower priority. When the CPU determines all line-fill buffers (LFB) are full,
it can simply ignore new prefetch requests. This happens more often in small unrolls than in large ones - on the latter,
more loads are filled before new prefetches are requested.
* The explicit loads (no_prefetch benchmark) are a different story - the CPU can't ignore them, so it has to try to
execute them. As a result, it might get stuck waiting for a LFB. It'd be interesting to extend the no_prefetch runs,
wrapping the volatile ugliness with some abstracting function.
* The number of LFBs in the test CPU is 16, which explains why it performs best. This LFB is Intel's terminology of
MSHR, so it's the same thing.