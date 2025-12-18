#pragma once
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <io.h>  // _write, _open, _close
#include <fcntl.h> // _O_BINARY, _O_WRONLY
#include <sys/stat.h>

#include "FastLoggerSetting.h"

class FastLogStream;

#if FASTLOG_TS_CHRONO
#include <chrono>
#endif

#if FASTLOG_TS_TSC
#include <intrin.h>
#endif


class FastLogger
{
    friend class FastLogStream;
public:
    FastLogger(std::string path);
    ~FastLogger();

    template<typename... Args>
    static inline void printf(const char* fmt, Args... args);
    static std::string getCurrentFormattedTime();

private:
    static void init(const char* path);
    static void init(std::string path);
    static void shutdown();

    static void writeRaw(const char* data, size_t len);
    static void writeStr(const char* s);

private:
    static constexpr size_t BUF_SIZE = 128*1024; // 1 MB = 1024*1024 = 1<<20

    static int fd;
    static char buffer[BUF_SIZE];
    static size_t pos;

    static void flush();

#if FASTLOG_TIMESTAMP
    static inline void writeTimestamp();
#endif
};
extern FastLogger flogger;
