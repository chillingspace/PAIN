#pragma once
#include "pch.h"

namespace PAIN::Util {
    inline std::string make_uuid_v4() {
        static thread_local std::mt19937_64 rng{ std::random_device{}() };
        std::uniform_int_distribution<uint64_t> dist;
        auto a = dist(rng), b = dist(rng);
        // version (4) + variant (10x)
        a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(8) << ((a >> 32) & 0xFFFFFFFFULL) << "-"
            << std::setw(4) << ((a >> 16) & 0xFFFFULL) << "-"
            << std::setw(4) << (a & 0xFFFFULL) << "-"
            << std::setw(4) << ((b >> 48) & 0xFFFFULL) << "-"
            << std::setw(12) << (b & 0xFFFFFFFFFFFFULL);
        return oss.str();
    }
}
