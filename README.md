# cpu-par-talk
Code examples for sequential code parallelism talk

## Installing google benchmark

### Ubuntu

```
sudo apt install libbenchmark-dev
```

### Oracle linux via EPEL - maybe works

```
sudo dnf install epel-release
sudo dnf search benchmark | grep google
```
If found `benchmark-devel` or so, install it.

### Oracle linux from source

```
git clone https://github.com/google/benchmark.git
cd benchmark && cmake -E make_directory build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on ..
make -j$(nproc)
sudo make install
```
