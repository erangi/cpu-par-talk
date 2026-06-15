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

The following run was done using a GCC-16 build, whose std::reduce implementation is apparently much better.

```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./simd-bench
generating data...
data vector size: 1024
data elements: -412, 397, 22, ..., -229
2026-06-12T08:45:55+00:00
Running ./simd-bench
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.07, 0.03, 0.04
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations
----------------------------------------------------------------------------
BM_findMax_std                           594 ns          594 ns      1177689
BM_findMax_reduce                       59.0 ns         59.0 ns     11869730
BM_findMax_intrinsics                   60.7 ns         60.7 ns     11533429
BM_findMax_intrinsics_unrolled          39.8 ns         39.8 ns     17597424
BM_findMaxAvx512                        58.8 ns         58.7 ns     11947788
BM_findMaxAvx2                          58.8 ns         58.8 ns     11899234
BM_findMaxScalar                         705 ns          704 ns       993680
BM_findMaxScalar_unrolled                245 ns          245 ns      2855917
BM_findMax_simd_portable                61.2 ns         61.1 ns     11451803
BM_findMax_simd_portable_unrolled       39.6 ns         39.6 ns     17680522
BM_findMax_simd_avx512                  61.4 ns         61.4 ns     11405297
BM_findMax_simd_avx512_unrolled         39.5 ns         39.5 ns     17731505
```

The result of `BM_findMaxAvx2` above (and the build output) indicate it's actually using AVX-512, despite the limiting target.
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

## TODO
* Re-run SIMD with `sudo taskset -c 0` to ensure it's scheduled on a high-priority core.
* Refactor MLP benchmark to use two kinds of "early load" abstractions, one with prefetch and one with volatile.
Run both and compare.

## Server setup

```
sudo apt update
sudo apt install linux-tools-$(uname -r) linux-tools-generic -y
sudo apt install htop -y
sudo apt install gcc-16 g++-16
g++-16 --version
sudo mkdir -p /sys/kernel/debug
sudo mount -t debugfs nodev /sys/kernel/debug
mount | grep -E 'debugfs|tracefs'
export PERF_DEBUGFS_DIR=/sys/kernel/debug
export PERF_TRACEFS_DIR=/sys/kernel/tracing
sudo apt install libbenchmark-dev
sudo apt install power-profiles-daemon -y
sudo powerprofilesctl set performance
```

```
scp -i ~/.ssh/id_ed25519 -r projects/cpu-par-talk ubuntu@131.153.231.25:.
```

After copying the project to the server:
```
./build.sh
sudo taskset -c 0 ./simd-bench
g++-16 -std=c++26 -O3 -march=native -mprefer-vector-width=512 -fopt-info-vec-optimized -DNDEBUG mlp-bench-2.cpp -lbenchmark -lpthread -o mlp-bench-2
sudo taskset -c 0 ./mlp-bench-2
```

## Final runs

Note: the `Library was built as DEBUG` comment refers to the google benchmark library, not the benchmarked code itself.
The executable is properly built as optimized release mode.

### MLP-2 preload by prefetch - meta-programming unrolling
```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./mlp-bench-2
### Using preload by prefetch ###
2026-06-13T10:17:32+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.15, 0.57, 0.53
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
-----------------------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------------------
BM_Serial/iterations:8               1777214638 ns   1776400191 ns            8 Branch_Mispredictions=15.4245M Core_Stall_On_Mem=12.5195M Instructions_Run=870.011M L1_Cache_Load_Misses=221.406M L1_Pending_Misses=77.3671G Line_Fill_Buf_Full=3.4141G Prefetches_Dropped=0
BM_BatchedGen1/iterations:8          1817937180 ns   1817070178 ns            8 Branch_Mispredictions=15.3783M Core_Stall_On_Mem=7.47696M Instructions_Run=953.897M L1_Cache_Load_Misses=223.058M L1_Pending_Misses=78.6726G Line_Fill_Buf_Full=3.43349G Prefetches_Dropped=14.7104M
BM_BatchedGen2/iterations:8          1653978473 ns   1653206399 ns            8 Branch_Mispredictions=15.291M Core_Stall_On_Mem=27.3107M Instructions_Run=917.197M L1_Cache_Load_Misses=219.774M L1_Pending_Misses=72.4448G Line_Fill_Buf_Full=3.20282G Prefetches_Dropped=14.8204M
BM_BatchedGen4/iterations:8          1519689901 ns   1518974663 ns            8 Branch_Mispredictions=15.393M Core_Stall_On_Mem=4.11317M Instructions_Run=896.225M L1_Cache_Load_Misses=217.412M L1_Pending_Misses=72.3201G Line_Fill_Buf_Full=3.5846G Prefetches_Dropped=17.0201M
BM_BatchedGen8/iterations:8          1384353749 ns   1383694293 ns            8 Branch_Mispredictions=15.4655M Core_Stall_On_Mem=6.70554M Instructions_Run=887.05M L1_Cache_Load_Misses=210.238M L1_Pending_Misses=66.628G Line_Fill_Buf_Full=3.32699G Prefetches_Dropped=13.7382M
BM_BatchedGen16/iterations:8         1366040454 ns   1365383736 ns            8 Branch_Mispredictions=16.3837M Core_Stall_On_Mem=3.33375M Instructions_Run=887.05M L1_Cache_Load_Misses=216.455M L1_Pending_Misses=68.2208G Line_Fill_Buf_Full=3.42666G Prefetches_Dropped=11.7873M
BM_BatchedGen32/iterations:8         1358397925 ns   1357771923 ns            8 Branch_Mispredictions=16.4473M Core_Stall_On_Mem=2.81563M Instructions_Run=889.016M L1_Cache_Load_Misses=218.292M L1_Pending_Misses=67.0604G Line_Fill_Buf_Full=3.46374G Prefetches_Dropped=10.2826M
BM_BatchedGen64/iterations:8         1467081231 ns   1466420186 ns            8 Branch_Mispredictions=17.7358M Core_Stall_On_Mem=4.49222M Instructions_Run=890.982M L1_Cache_Load_Misses=227.624M L1_Pending_Misses=70.3714G Line_Fill_Buf_Full=3.64529G Prefetches_Dropped=9.21095M
BM_Batched4/iterations:8             1539931967 ns   1539221863 ns            8 Branch_Mispredictions=15.4096M Core_Stall_On_Mem=9.38281M Instructions_Run=896.225M L1_Cache_Load_Misses=217.152M L1_Pending_Misses=70.698G Line_Fill_Buf_Full=3.39939G Prefetches_Dropped=16.1187M
BM_Batched4_no_prefetch/iterations:8 1515003606 ns   1514320315 ns            8 Branch_Mispredictions=15.269M Core_Stall_On_Mem=7.13835M Instructions_Run=875.253M L1_Cache_Load_Misses=216.105M L1_Pending_Misses=71.4235G Line_Fill_Buf_Full=3.41375G Prefetches_Dropped=0
BM_Batched8/iterations:8             1387699708 ns   1387084109 ns            8 Branch_Mispredictions=15.3217M Core_Stall_On_Mem=4.12996M Instructions_Run=887.05M L1_Cache_Load_Misses=212.166M L1_Pending_Misses=66.9872G Line_Fill_Buf_Full=3.36651G Prefetches_Dropped=13.3589M
```

### MLP-2 preload by volatile read - meta-programming unrolling
```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./mlp-bench-2
### Using preload by volatile read ###
2026-06-13T10:33:14+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.15, 0.60, 0.70
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
-----------------------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------------------
BM_Serial/iterations:8               1763985461 ns   1763185085 ns            8 Branch_Mispredictions=15.3137M Core_Stall_On_Mem=14.9514M Instructions_Run=870.011M L1_Cache_Load_Misses=218.414M L1_Pending_Misses=76.0956G Line_Fill_Buf_Full=3.33085G Prefetches_Dropped=0
BM_BatchedGen1/iterations:8          1728774960 ns   1727948534 ns            8 Branch_Mispredictions=15.339M Core_Stall_On_Mem=10.7289M Instructions_Run=911.954M L1_Cache_Load_Misses=219.147M L1_Pending_Misses=76.6864G Line_Fill_Buf_Full=3.39731G Prefetches_Dropped=0
BM_BatchedGen2/iterations:8          2135832295 ns   2134822274 ns            8 Branch_Mispredictions=15.3112M Core_Stall_On_Mem=4.4407M Instructions_Run=901.468M L1_Cache_Load_Misses=228.816M L1_Pending_Misses=84.1256G Line_Fill_Buf_Full=3.43843G Prefetches_Dropped=0
BM_BatchedGen4/iterations:8          1560007932 ns   1559290799 ns            8 Branch_Mispredictions=15.1967M Core_Stall_On_Mem=26.9663M Instructions_Run=896.225M L1_Cache_Load_Misses=221.554M L1_Pending_Misses=68.5172G Line_Fill_Buf_Full=3.05401G Prefetches_Dropped=0
BM_BatchedGen8/iterations:8          1407078062 ns   1406403456 ns            8 Branch_Mispredictions=16.3497M Core_Stall_On_Mem=8.71861M Instructions_Run=898.846M L1_Cache_Load_Misses=213.799M L1_Pending_Misses=69.2855G Line_Fill_Buf_Full=3.30196G Prefetches_Dropped=0
BM_BatchedGen16/iterations:8         1369091634 ns   1368455508 ns            8 Branch_Mispredictions=16.597M Core_Stall_On_Mem=2.77708M Instructions_Run=902.778M L1_Cache_Load_Misses=218.188M L1_Pending_Misses=71.2086G Line_Fill_Buf_Full=3.41852G Prefetches_Dropped=0
BM_BatchedGen32/iterations:8         1355120339 ns   1354500997 ns            8 Branch_Mispredictions=16.0475M Core_Stall_On_Mem=2.65907M Instructions_Run=907.366M L1_Cache_Load_Misses=216.035M L1_Pending_Misses=69.5703G Line_Fill_Buf_Full=3.38313G Prefetches_Dropped=0
BM_BatchedGen64/iterations:8         1454906315 ns   1454191717 ns            8 Branch_Mispredictions=17.7083M Core_Stall_On_Mem=15.8981M Instructions_Run=930.959M L1_Cache_Load_Misses=226.843M L1_Pending_Misses=74.7065G Line_Fill_Buf_Full=3.57055G Prefetches_Dropped=0
BM_Batched4/iterations:8             1481310666 ns   1480625225 ns            8 Branch_Mispredictions=15.2312M Core_Stall_On_Mem=7.71045M Instructions_Run=896.225M L1_Cache_Load_Misses=215.293M L1_Pending_Misses=69.736G Line_Fill_Buf_Full=3.35967G Prefetches_Dropped=0
BM_Batched4_no_prefetch/iterations:8 1519725871 ns   1519003272 ns            8 Branch_Mispredictions=15.2994M Core_Stall_On_Mem=7.62521M Instructions_Run=875.253M L1_Cache_Load_Misses=217.128M L1_Pending_Misses=73.4385G Line_Fill_Buf_Full=3.55968G Prefetches_Dropped=0
BM_Batched8/iterations:8             1396417107 ns   1395748671 ns            8 Branch_Mispredictions=15.8218M Core_Stall_On_Mem=7.99714M Instructions_Run=898.846M L1_Cache_Load_Misses=212.765M L1_Pending_Misses=69.1615G Line_Fill_Buf_Full=3.30764G Prefetches_Dropped=0
```

### MLP-2 preload by prefetch - pragma unrolling
```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./mlp-bench-2
### Using preload by prefetch ###
2026-06-13T11:35:00+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.17, 0.51, 0.69
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
-----------------------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------------------
BM_Serial/iterations:8               1775404089 ns   1774578394 ns            8 Branch_Mispredictions=15.4484M Core_Stall_On_Mem=11.8097M Instructions_Run=870.011M L1_Cache_Load_Misses=220.826M L1_Pending_Misses=77.3541G Line_Fill_Buf_Full=3.40752G Prefetches_Dropped=0
BM_BatchedGen1/iterations:8          1831396307 ns   1830511223 ns            8 Branch_Mispredictions=15.4032M Core_Stall_On_Mem=7.62476M Instructions_Run=953.897M L1_Cache_Load_Misses=224.916M L1_Pending_Misses=79.4821G Line_Fill_Buf_Full=3.4692G Prefetches_Dropped=14.6498M
BM_BatchedGen2/iterations:8          1661120446 ns   1660331652 ns            8 Branch_Mispredictions=15.314M Core_Stall_On_Mem=25.497M Instructions_Run=917.197M L1_Cache_Load_Misses=220.011M L1_Pending_Misses=74.1744G Line_Fill_Buf_Full=3.28355G Prefetches_Dropped=17.0084M
BM_BatchedGen4/iterations:8          1520271729 ns   1519572883 ns            8 Branch_Mispredictions=15.3469M Core_Stall_On_Mem=4.47024M Instructions_Run=896.225M L1_Cache_Load_Misses=216.478M L1_Pending_Misses=71.8926G Line_Fill_Buf_Full=3.55999G Prefetches_Dropped=17.0711M
BM_BatchedGen8/iterations:8          1383168601 ns   1382513589 ns            8 Branch_Mispredictions=15.5989M Core_Stall_On_Mem=6.5762M Instructions_Run=887.05M L1_Cache_Load_Misses=211.023M L1_Pending_Misses=66.7999G Line_Fill_Buf_Full=3.34219G Prefetches_Dropped=13.6659M
BM_BatchedGen16/iterations:8         1370101381 ns   1369465562 ns            8 Branch_Mispredictions=16.2242M Core_Stall_On_Mem=3.22851M Instructions_Run=887.05M L1_Cache_Load_Misses=216.831M L1_Pending_Misses=68.3803G Line_Fill_Buf_Full=3.43668G Prefetches_Dropped=11.7725M
BM_BatchedGen32/iterations:8         1357591842 ns   1356942610 ns            8 Branch_Mispredictions=16.4247M Core_Stall_On_Mem=2.82272M Instructions_Run=889.016M L1_Cache_Load_Misses=219.787M L1_Pending_Misses=67.6181G Line_Fill_Buf_Full=3.49139G Prefetches_Dropped=10.2718M
BM_BatchedGen64/iterations:8         1421706539 ns   1421018925 ns            8 Branch_Mispredictions=18.6483M Core_Stall_On_Mem=10.6219M Instructions_Run=890.163M L1_Cache_Load_Misses=225.139M L1_Pending_Misses=69.166G Line_Fill_Buf_Full=3.56592G Prefetches_Dropped=9.06339M
BM_Batched4/iterations:8             1547394909 ns   1546655090 ns            8 Branch_Mispredictions=15.4623M Core_Stall_On_Mem=8.89083M Instructions_Run=896.225M L1_Cache_Load_Misses=218.612M L1_Pending_Misses=71.5131G Line_Fill_Buf_Full=3.4429G Prefetches_Dropped=16.0419M
BM_Batched4_no_prefetch/iterations:8 1510929481 ns   1510212560 ns            8 Branch_Mispredictions=15.1797M Core_Stall_On_Mem=5.67274M Instructions_Run=875.253M L1_Cache_Load_Misses=216.275M L1_Pending_Misses=71.9952G Line_Fill_Buf_Full=3.4736G Prefetches_Dropped=0
BM_Batched8/iterations:8             1394607463 ns   1393954211 ns            8 Branch_Mispredictions=15.3455M Core_Stall_On_Mem=3.84415M Instructions_Run=887.05M L1_Cache_Load_Misses=213.009M L1_Pending_Misses=67.4298G Line_Fill_Buf_Full=3.38931G Prefetches_Dropped=13.3304M
```

### MLP-2 preload by volatile read - pragma unrolling
```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./mlp-bench-2
### Using preload by volatile read ###
2026-06-13T11:18:18+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.22, 0.59, 0.62
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
-----------------------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------------------
BM_Serial/iterations:8               1821429731 ns   1820561408 ns            8 Branch_Mispredictions=15.3767M Core_Stall_On_Mem=17.1629M Instructions_Run=870.011M L1_Cache_Load_Misses=221.79M L1_Pending_Misses=77.5154G Line_Fill_Buf_Full=3.34475G Prefetches_Dropped=0
BM_BatchedGen1/iterations:8          1808233134 ns   1807387171 ns            8 Branch_Mispredictions=15.7222M Core_Stall_On_Mem=11.4666M Instructions_Run=911.954M L1_Cache_Load_Misses=221.233M L1_Pending_Misses=77.3579G Line_Fill_Buf_Full=3.34533G Prefetches_Dropped=0
BM_BatchedGen2/iterations:8          1522885923 ns   1522197753 ns            8 Branch_Mispredictions=15.1741M Core_Stall_On_Mem=3.94021M Instructions_Run=901.468M L1_Cache_Load_Misses=211.124M L1_Pending_Misses=69.1358G Line_Fill_Buf_Full=3.34754G Prefetches_Dropped=0
BM_BatchedGen4/iterations:8          1542876461 ns   1542166948 ns            8 Branch_Mispredictions=15.2212M Core_Stall_On_Mem=7.33187M Instructions_Run=896.225M L1_Cache_Load_Misses=216.633M L1_Pending_Misses=72.6575G Line_Fill_Buf_Full=3.49487G Prefetches_Dropped=0
BM_BatchedGen8/iterations:8          1401102701 ns   1400417708 ns            8 Branch_Mispredictions=15.6734M Core_Stall_On_Mem=7.54914M Instructions_Run=898.846M L1_Cache_Load_Misses=213.262M L1_Pending_Misses=69.055G Line_Fill_Buf_Full=3.23704G Prefetches_Dropped=0
BM_BatchedGen16/iterations:8         1358342636 ns   1357684016 ns            8 Branch_Mispredictions=16.6589M Core_Stall_On_Mem=2.43188M Instructions_Run=902.778M L1_Cache_Load_Misses=217.069M L1_Pending_Misses=70.9046G Line_Fill_Buf_Full=3.39569G Prefetches_Dropped=0
BM_BatchedGen32/iterations:8         1353113952 ns   1352447828 ns            8 Branch_Mispredictions=16.806M Core_Stall_On_Mem=2.66187M Instructions_Run=907.366M L1_Cache_Load_Misses=216.995M L1_Pending_Misses=69.883G Line_Fill_Buf_Full=3.37418G Prefetches_Dropped=0
BM_BatchedGen64/iterations:8         1452807586 ns   1452086875 ns            8 Branch_Mispredictions=18.0406M Core_Stall_On_Mem=13.9568M Instructions_Run=909.496M L1_Cache_Load_Misses=224.919M L1_Pending_Misses=74.2918G Line_Fill_Buf_Full=3.53445G Prefetches_Dropped=0
BM_Batched4/iterations:8             1528029525 ns   1527323219 ns            8 Branch_Mispredictions=15.2924M Core_Stall_On_Mem=14.2487M Instructions_Run=896.225M L1_Cache_Load_Misses=217.742M L1_Pending_Misses=69.8379G Line_Fill_Buf_Full=3.27478G Prefetches_Dropped=0
BM_Batched4_no_prefetch/iterations:8 1609655924 ns   1608914470 ns            8 Branch_Mispredictions=15.2812M Core_Stall_On_Mem=19.2969M Instructions_Run=875.254M L1_Cache_Load_Misses=218.994M L1_Pending_Misses=75.0775G Line_Fill_Buf_Full=3.50854G Prefetches_Dropped=0
BM_Batched8/iterations:8             1407459636 ns   1406789919 ns            8 Branch_Mispredictions=15.9962M Core_Stall_On_Mem=5.01338M Instructions_Run=898.846M L1_Cache_Load_Misses=213.193M L1_Pending_Misses=68.8755G Line_Fill_Buf_Full=3.29571G Prefetches_Dropped=0
```

### MLP-2 with preload completely disabled
```
ubuntu@s5-x6-c3-medium-us-sw1:~/cpu-par-talk$ sudo taskset -c 0 ./mlp-bench-2
### Using preload by prefetch ###
2026-06-12T11:33:53+00:00
Running ./mlp-bench-2
Run on (48 X 4200 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 2048 KiB (x24)
  L3 Unified 147456 KiB (x1)
Load Average: 0.02, 0.01, 0.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------
BM_Serial               1857760918 ns   1856877596 ns            1 Branch_Mispredictions=15.7096M Core_Stall_On_Mem=15.2985M Instructions_Run=870.017M L1_Cache_Load_Misses=222.451M L1_Pending_Misses=78.3872G Line_Fill_Buf_Full=3.41617G Prefetches_Dropped=0
BM_BatchedGen1          1861553171 ns   1860666157 ns            1 Branch_Mispredictions=15.5015M Core_Stall_On_Mem=15.0218M Instructions_Run=870.017M L1_Cache_Load_Misses=222.718M L1_Pending_Misses=78.5844G Line_Fill_Buf_Full=3.41903G Prefetches_Dropped=0
BM_BatchedGen2          1662950629 ns   1662192833 ns            1 Branch_Mispredictions=15.2469M Core_Stall_On_Mem=28.9879M Instructions_Run=854.288M L1_Cache_Load_Misses=220.695M L1_Pending_Misses=72.2333G Line_Fill_Buf_Full=3.20904G Prefetches_Dropped=0
BM_BatchedGen4          2033024836 ns   2032084559 ns            1 Branch_Mispredictions=15.5994M Core_Stall_On_Mem=23.8581M Instructions_Run=846.424M L1_Cache_Load_Misses=240.035M L1_Pending_Misses=84.0889G Line_Fill_Buf_Full=3.60255G Prefetches_Dropped=0
BM_BatchedGen8          2107629831 ns   2106627324 ns            1 Branch_Mispredictions=17.3668M Core_Stall_On_Mem=22.6661M Instructions_Run=843.803M L1_Cache_Load_Misses=240.365M L1_Pending_Misses=86.1431G Line_Fill_Buf_Full=3.62648G Prefetches_Dropped=0
BM_BatchedGen16         2194959078 ns   2193938678 ns            1 Branch_Mispredictions=16.3838M Core_Stall_On_Mem=28.2937M Instructions_Run=841.181M L1_Cache_Load_Misses=240.943M L1_Pending_Misses=86.0062G Line_Fill_Buf_Full=3.5185G Prefetches_Dropped=0
BM_BatchedGen32         2354540793 ns   2353419791 ns            1 Branch_Mispredictions=17.6582M Core_Stall_On_Mem=18.1096M Instructions_Run=839.871M L1_Cache_Load_Misses=245.792M L1_Pending_Misses=83.7373G Line_Fill_Buf_Full=3.00569G Prefetches_Dropped=0
BM_BatchedGen64         2496321309 ns   2495118588 ns            1 Branch_Mispredictions=18.2945M Core_Stall_On_Mem=4.08405M Instructions_Run=839.216M L1_Cache_Load_Misses=245.58M L1_Pending_Misses=83.3781G Line_Fill_Buf_Full=2.6423G Prefetches_Dropped=0
BM_Batched4             2071046461 ns   2070056147 ns            1 Branch_Mispredictions=15.9201M Core_Stall_On_Mem=15.9963M Instructions_Run=846.424M L1_Cache_Load_Misses=234.368M L1_Pending_Misses=84.9835G Line_Fill_Buf_Full=3.56682G Prefetches_Dropped=0
BM_Batched4_no_prefetch 1521080373 ns   1520408576 ns            1 Branch_Mispredictions=15.6759M Core_Stall_On_Mem=8.82803M Instructions_Run=875.259M L1_Cache_Load_Misses=215.723M L1_Pending_Misses=71.7369G Line_Fill_Buf_Full=3.44604G Prefetches_Dropped=0
BM_Batched8             2236128683 ns   2235072111 ns            1 Branch_Mispredictions=15.7746M Core_Stall_On_Mem=36.4878M Instructions_Run=843.803M L1_Cache_Load_Misses=234.961M L1_Pending_Misses=83.0407G Line_Fill_Buf_Full=3.16309G Prefetches_Dropped=0
```

The source of the latency in the `BM_BatchedGen` runs is the array used to gather results. With a single or two cells,
it was converted to registers. Above that, using it involves additional memory (L1) accesses, which are slower than
using registers.