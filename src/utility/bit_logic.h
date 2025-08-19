#ifndef BIT_LOGIC_H
#define BIT_LOGIC_H

#include <cstdint>

#if defined(_MSC_VER)
    #include <intrin.h>

    inline int ctz32(uint32_t x) {
        unsigned long idx;
        _BitScanForward(&idx, x); // undefined if x==0
        return static_cast<int>(idx);
    }

    inline int ctz64(uint64_t x) {
        unsigned long idx;
    #if defined(_M_X64) || defined(_M_AMD64)
        _BitScanForward64(&idx, x); // undefined if x==0
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

    inline int popcount32(uint32_t x) {
        return __popcnt(x);
    }

    inline int popcount64(uint64_t x) {
    #if defined(_M_X64) || defined(_M_AMD64)
        return static_cast<int>(__popcnt64(x));
    #else
        return __popcnt(static_cast<uint32_t>(x)) +
               __popcnt(static_cast<uint32_t>(x >> 32));
    #endif
    }

#elif defined(__GNUC__) || defined(__clang__)

    inline int ctz32(uint32_t x) {
        return __builtin_ctz(x); // undefined if x==0
    }

    inline int ctz64(uint64_t x) {
        return __builtin_ctzll(x); // undefined if x==0
    }

    inline int popcount32(uint32_t x) {
        return __builtin_popcount(x);
    }

    inline int popcount64(uint64_t x) {
        return __builtin_popcountll(x);
    }

#else // Generic portable fallback

    inline int ctz32(uint32_t x) {
        int idx = 0;
        while ((x & 1u) == 0u) { x >>= 1; ++idx; }
        return idx;
    }

    inline int ctz64(uint64_t x) {
        int idx = 0;
        while ((x & 1ull) == 0ull) { x >>= 1; ++idx; }
        return idx;
    }

    inline int popcount32(uint32_t x) {
        int count = 0;
        while (x) { x &= (x - 1); ++count; }
        return count;
    }

    inline int popcount64(uint64_t x) {
        int count = 0;
        while (x) { x &= (x - 1); ++count; }
        return count;
    }

#endif

#endif // BIT_LOGIC_H
