#include "stdafx.h"
#include "FastLogger.h"
#include <thread>
#include <chrono>

int FastLogger::fd = -1;
char FastLogger::buffer[FastLogger::BUF_SIZE + 1024];
size_t FastLogger::pos = 0;
FastLogger flogger("C:\\Chimera\\Chimera-Cryo\\Log\\" + FastLogger::getCurrentFormattedTime() + ".log");

FastLogger::FastLogger(std::string path)
{
    init(path);
#if FASTLOG_TS_TSC
    calibrate_tsc();
#endif
    FastLogger::printf("Start Logging at %s \n", std::string(FastLogger::getCurrentFormattedTime()).c_str());
    flush();
}

FastLogger::~FastLogger()
{
    shutdown();
}

void FastLogger::init(const char* path)
{
    fd = _open(path,
        _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
        _S_IREAD | _S_IWRITE);
}

void FastLogger::init(std::string path)
{
    init(path.c_str());
}

void FastLogger::shutdown()
{
    flush();
    if (fd >= 0)
        _close(fd);
}

void FastLogger::flush()
{
    if (pos) {
        //FastLogger::printf("Flushed\n");
#if FASTLOG_TIMESTAMP
        writeTimestamp(false);
#endif
        writeRaw("Flushed\n", 8, false);
        _write(fd, buffer, (unsigned)pos);
    }
    pos = 0;
}

#if FASTLOG_TS_TSC
void FastLogger::calibrate_tsc()
{
    using clock = std::chrono::high_resolution_clock;

    // Warm up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto t0 = clock::now();
    uint64_t c0 = __rdtsc();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto t1 = clock::now();
    uint64_t c1 = __rdtsc();

    auto dt_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    uint64_t dc = c1 - c0;

    tsc_cycles_per_sec = (dc * 1000000000ULL) / dt_ns;

    double ghz = (double)tsc_cycles_per_sec / 1e9;

    resetTimeStamp();

    FastLogger::printf("TSC calibrated: %.6f GHz\n", ghz);
    flush();
}
#endif

void FastLogger::writeRaw(const char* data, size_t len, bool doFlush)
{
    if (doFlush && pos + len > BUF_SIZE)
        flush();

    std::memcpy(buffer + pos, data, len);
    pos += len;
}

void FastLogger::writeStr(const char* s)
{
    writeRaw(s, std::strlen(s));
}

std::string FastLogger::getCurrentFormattedTime()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_struct = *std::localtime(&now);
    char buffer[80];
    // Format string: "%Y-%m-%d %H:%M:%S" gives "YYYY-MM-DD HH:MM:SS"
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H%M%S", &tm_struct);
    return buffer;
}

void FastLogger::resetTimeStamp()
{
#if FASTLOG_TS_TSC
    start_rdtsc = __rdtsc();
#endif
}

template<typename... Args>
inline void FastLogger::printf(const char* fmt, Args... args)
{
#if FASTLOG_TIMESTAMP
    writeTimestamp();
#endif

#if FASTLOG_TEXT
    char small[256];
    int n = std::snprintf(small, sizeof(small), fmt, args...);

    if (n < (int)sizeof(small)) {
        writeRaw(small, n);
    }
    else {
        char* big = (char*)alloca(n + 1);
        std::snprintf(big, n + 1, fmt, args...);
        writeRaw(big, n);
    }
#else
    // Binary mode: raw struct only (printf disabled)
    (void)fmt;
#endif
}

#if FASTLOG_TIMESTAMP && FASTLOG_TS_TSC
uint64_t FastLogger::tsc_cycles_per_sec = 0;
uint64_t FastLogger::start_rdtsc = 0;
inline void FastLogger::writeTimestamp(bool doFlush)
{
    uint64_t cycles = __rdtsc();
    double ns = double(cycles - start_rdtsc) * 1e9 / tsc_cycles_per_sec;
    char buf[64];
    int n = std::snprintf(buf, sizeof(buf), "[%llu] [%17.6f ms] ",
        (unsigned long long)cycles, ns/1e6); // %17 corresponds to the precision of a double to be roughly 16 digits
    writeRaw(buf, n, doFlush);
}
#endif

#if FASTLOG_TIMESTAMP && FASTLOG_TS_CHRONO
inline void FastLogger::writeTimestamp()
{
    using clock = std::chrono::high_resolution_clock;
    auto now = clock::now().time_since_epoch();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    char buf[40];
    int n = std::snprintf(buf, sizeof(buf), "[%lld ns] ",
        (long long)ns);
    writeRaw(buf, n);
}
#endif