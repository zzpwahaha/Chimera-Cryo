#include "stdafx.h"
#include "FastLogger.h"

int FastLogger::fd = -1;
char FastLogger::buffer[FastLogger::BUF_SIZE];
size_t FastLogger::pos = 0;
FastLogger flogger("C:\\Chimera\\Chimera-Cryo\\Log\\" + FastLogger::getCurrentFormattedTime() + ".log");

FastLogger::FastLogger(std::string path)
{
    init(path);
    printf("Start Logging at %s \n", FastLogger::getCurrentFormattedTime());
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

inline void FastLogger::flush()
{
    if (pos)
        _write(fd, buffer, (unsigned)pos);
    pos = 0;
}

inline void FastLogger::writeRaw(const char* data, size_t len)
{
    if (pos + len > BUF_SIZE)
        flush();

    std::memcpy(buffer + pos, data, len);
    pos += len;
}

inline void FastLogger::writeStr(const char* s)
{
    writeRaw(s, std::strlen(s));
}

std::string FastLogger::getCurrentFormattedTime()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_struct = *std::localtime(&now);
    char buffer[80];
    // Format string: "%Y-%m-%d %H:%M:%S" gives "YYYY-MM-DD HH:MM:SS"
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_struct);
    return buffer;
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
inline void FastLogger::writeTimestamp()
{
    uint64_t t = __rdtsc();
    char buf[32];
    int n = std::snprintf(buf, sizeof(buf), "[%llu] ",
        (unsigned long long)t);
    writeRaw(buf, n);
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