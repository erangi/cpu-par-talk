# cpu-par-talk
Code examples for my Core C++ 2025 talk - [Parallel Paths: From SIMD Lanes to Memory Highways in C++](https://corecpp.org/schedule_orig/#session-12)

## Installing google benchmark

### Ubuntu

```
sudo apt install libbenchmark-dev
```

### Oracle linux 8.5

```
sudo dnf install google-benchmark-devel
```

## Running benchmarks

The SIMD benchmarks are in the `simd-bench` executable, and the MLP benchmarks are in the `mlp-bench` executable. Run them as is to execute all the benchmarks or add `--benchmark_filter=<filter>` to the command line to run just some of the benchmarks.
