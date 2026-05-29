#!/bin/bash

g++-16 -std=c++26 -O3 -march=native -mprefer-vector-width=512 -fopt-info-vec-optimized -DNDEBUG simd-bench.cpp -lbenchmark -lpthread -o simd-bench
g++-16 -std=c++26 -O3 -march=native -mprefer-vector-width=512 -fopt-info-vec-optimized -DNDEBUG mlp-bench.cpp -lbenchmark -lpthread -o mlp-bench
g++-16 -std=c++26 -O3 -march=native -mprefer-vector-width=512 -fopt-info-vec-optimized -DNDEBUG mlp-bench-2.cpp -lbenchmark -lpthread -o mlp-bench-2