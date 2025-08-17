#ifndef BIT_LOGIC_H
#define BIT_LOGIC_H

#include <cstdint>

#if defined(_MSC_VER)
    #include <intrin.h>

    inline int ctz32(uint32_t x) {
        unsigned long idx;
        _BitScanForward(&idx, x);   // undefined if x==0, but we never call on 0
        return static_cast<int>(idx);
    }
#elif defined(__GNUC__) || defined(__clang__)
    inline int ctz32(uint32_t x) {
        return __builtin_ctz(x);    // undefined if x==0
    }
#else
    inline int ctz32(uint32_t x) {
        int idx = 0;
        while ((x & 1u) == 0u) { x >>= 1; ++idx; }
        return idx;
    }
#endif

#if defined(_MSC_VER)
    #include <intrin.h>

    inline int ctz64(uint64_t x) {
        unsigned long idx;
    #if defined(_M_X64) || defined(_M_AMD64)
        _BitScanForward64(&idx, x);    // undefined if x==0, but never called with 0
    #else
        // On 32-bit MSVC, fall back to two 32-bit scans
        if (static_cast<uint32_t>(x) != 0) {
            _BitScanForward(&idx, static_cast<uint32_t>(x));
            return static_cast<int>(idx);
        } else {
            _BitScanForward(&idx, static_cast<uint32_t>(x >> 32));
            return static_cast<int>(idx + 32);
        }
    #endif
        return static_cast<int>(idx);
    }

#elif defined(__GNUC__) || defined(__clang__)
    inline int ctz64(uint64_t x) {
        return __builtin_ctzll(x);     // undefined if x==0
    }

#else
    inline int ctz64(uint64_t x) {
        int idx = 0;
        while ((x & 1ull) == 0ull) {
            x >>= 1;
            ++idx;
        }
        return idx;
    }
#endif

#endif