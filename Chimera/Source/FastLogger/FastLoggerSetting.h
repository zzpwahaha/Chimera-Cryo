#pragma once

// ===== MODE SELECTION =====
#define FASTLOG_TEXT        1   // 1 = text, 0 = binary
#define FASTLOG_TIMESTAMP   1   // 1 = enable timestamps
#define FASTLOG_TS_CHRONO   0   // 1 = high_resolution_clock
#define FASTLOG_TS_TSC      1   // 1 = rdtsc (fastest)

// Safety checks
#if FASTLOG_TS_CHRONO && FASTLOG_TS_TSC
#error "Choose only one timestamp source"
#endif