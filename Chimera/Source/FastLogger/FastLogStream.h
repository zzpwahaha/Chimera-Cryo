#pragma once
#include "FastLogger.h"

class FastLogStream 
{
    public:
        inline FastLogStream& operator<<(const char* s)
        {
#if FASTLOG_TIMESTAMP
            FastLogger::writeTimestamp();
#endif
            FastLogger::writeStr(s);
            return *this;
        }

        inline FastLogStream& operator<<(int v)
        {
            char buf[32];
            int n = std::snprintf(buf, sizeof(buf), "%d", v);
            FastLogger::writeRaw(buf, n);
            return *this;
        }

        inline FastLogStream& operator<<(double v)
        {
            char buf[64];
            int n = std::snprintf(buf, sizeof(buf), "%.6f", v);
            FastLogger::writeRaw(buf, n);
            return *this;
        }

        inline FastLogStream& operator<<(char c)
        {
            FastLogger::writeRaw(&c, 1);
            return *this;
        }
};

extern FastLogStream flog;

