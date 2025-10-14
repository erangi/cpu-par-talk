#!/bin/bash

g++ -O3 -DNDEBUG simd-bench.cpp -lbenchmark -lpthread -o simd-bench
g++ -O3 -DNDEBUG mlp-bench.cpp -lbenchmark -lpthread -o mlp-bench