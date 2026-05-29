#!/bin/bash
# Server Setup Script for C++26 SIMD Benchmarking
# OS: Ubuntu 24.04.4
# The reserved server was s5.x6.c3.large (Single Intel Intel Xeon 6527P, 256 GB RAM DDR5, $1.29/hour @ phoenixnap.com)

set -e

# 1. Update and Install Base Tools
sudo apt update
sudo apt install -y build-essential flex bison libgmp-dev libmpfr-dev libmpc-dev \
    linux-tools-$(uname -r) linux-tools-generic libbenchmark-dev power-profiles-daemon

# 2. Install GCC 15 (Toolchain PPA)
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y gcc-15 g++-15

# 3. Configure Power Profile to Performance
sudo powerprofilesctl set performance

# 4. Setup Debug Filesystem for Perf
if ! mount | grep -q '/sys/kernel/debug'; then
    sudo mount -t debugfs nodev /sys/kernel/debug
fi

# 5. Configure Kernel Performance Settings
sudo sysctl -w kernel.perf_event_paranoid=0

echo "Setup complete."
echo "Use 'g++-15 -std=c++26 -O3 -march=sapphirerapids' for compilation."