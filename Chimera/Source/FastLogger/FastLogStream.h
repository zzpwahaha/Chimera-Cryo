#pragma once
#include "FastLogger.h"
#include <atomic>

class FastLogStream 
{
    private:
        std::atomic<bool> newline = true;
        inline void handleNewline() {
            if (newline) {
#if FASTLOG_TIMESTAMP
                FastLogger::writeTimestamp();
#endif
                newline = false;
            }
        }


    public:
        inline FastLogStream& operator<<(const char* s)
        {
            handleNewline();
            FastLogger::writeStr(s);
            return *this;
        }

        template<typename T,
            typename = std::enable_if_t<std::is_integral_v<T>>>
            inline FastLogStream& operator<<(T v) 
        {
            handleNewline();
            char buf[32];
            int n = 0;
            if constexpr (std::is_signed_v<T>) {
                // cast to long long to safely handle 64-bit signed integers
                n = std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
            }
            else {
                // cast to unsigned long long to safely handle 64-bit unsigned integers
                n = std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(v));
            }
            FastLogger::writeRaw(buf, n);
            return *this;
        }

        inline FastLogStream& operator<<(double v)
        {
            handleNewline();
            char buf[64];
            int n = std::snprintf(buf, sizeof(buf), "%.6f", v);
            FastLogger::writeRaw(buf, n);
            return *this;
        }

        inline FastLogStream& operator<<(char c)
        {
            handleNewline();
            FastLogger::writeRaw(&c, 1);
            return *this;
        }

        inline FastLogStream& operator<<(const std::string& s) {
            handleNewline();
            FastLogger::writeRaw(s.c_str(), s.size());
            return *this;
        }

        using StreamManipulator = FastLogStream& (*)(FastLogStream&);
        inline FastLogStream& operator<<(StreamManipulator manip) {
            return manip(*this);
        }

        inline static FastLogStream& endl(FastLogStream& stream) {
            FastLogger::writeRaw("\n", 1);
            stream.newline = true;
            return stream;
        }
};

inline FastLogStream flog;
inline auto fendl = FastLogStream::endl;

