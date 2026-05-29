#pragma once

#include <string>

#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <unistd.h>

enum class PmuEvent : uint64_t
{
    // Event: L1D_PEND_MISS.PENDING
    // Measures cycles * outstanding L1D data misses
    L1D_PENDING_MISSES = 0x0148,

    // Event: L1D.REPLACEMENT (or microarchitecture equivalent)
    // Tracks L1 Data Cache load misses
    L1D_CACHE_LOAD_MISSES = 0x0151,

    // Event Name: RESOURCE_STALLS.SCOREBOARD
    RESOURCE_STALLS_SCOREBOARD = 0x02a2,

    // Saturation Indicators -

    // Event Name: L1D_PEND_MISS.FB_FULL
    L1D_FB_FULL = 0x0248,

    // Event Name: SW_PREFETCH_ACCESS.NTA
    SW_PREFETCH_T0_HITS = 0x014c,

    // Event Name: INST_RETIRED.ANY
    INSTRUCTIONS_RETIRED = 0x00c0,

    // Event Name: BR_MISP_RETIRED.ALL_BRANCHES
    BRANCH_MISPREDICTIONS = 0x00c5,
};

class PerfCounter
{
public:
    // Bypasses names and opens raw Intel PMU hex values directly
    PerfCounter(PmuEvent event)
    {
        struct perf_event_attr pe {};
        pe.type = PERF_TYPE_RAW;
        pe.size = sizeof(struct perf_event_attr);

        // Cast the enum back to its underlying 64-bit value for the system call
        pe.config = static_cast<uint64_t>(event);

        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;

        fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) {
            throw std::string{"failed to open counter of event "} + std::to_string(pe.config);
        }
    }

    ~PerfCounter() { close(fd); }

    void start()
    {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }

    void stop()
    {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    }

    long long read_val()
    {
        long long count;
        read(fd, &count, sizeof(long long));
        return count;
    }

private:
    int fd = -1;

};
