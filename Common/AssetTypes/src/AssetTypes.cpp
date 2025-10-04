#include "AssetTypes.h"

#if defined(_WIN32)
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <uuid/uuid.h>
#endif

#include <random>

namespace PAIN {

    GUID GUID::Generate() noexcept {
        GUID guid;

#if defined(_WIN32)
        UUID uuid;
        if (UuidCreate(&uuid) == RPC_S_OK) {
            std::memcpy(guid.bytes, &uuid, 16);
            return guid;
        }
#elif defined(__linux__) || defined(__APPLE__)
        uuid_t uuid;
        uuid_generate(uuid);
        std::memcpy(guid.bytes, &uuid, 16);
        return guid;
#endif

        // Fallback: PRNG-based generation
        static thread_local std::random_device rd;
        static thread_local std::mt19937_64 rng(rd());
        static thread_local std::uniform_int_distribution<uint64_t> dist;

        uint64_t* parts = reinterpret_cast<uint64_t*>(guid.bytes);
        parts[0] = dist(rng);
        parts[1] = dist(rng);

        // Set version bits to 4 (random UUID ver 4)
        guid.bytes[6] = (guid.bytes[6] & 0x0F) | 0x40;
        guid.bytes[8] = (guid.bytes[8] & 0x3F) | 0x80;

        return guid;
    }

    GUID::GUID(const std::string& str) {
        static auto hexCharToInt = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return -1;
            };

        // Clear bytes
        std::fill(std::begin(bytes), std::end(bytes), 0);

        // Remove hyphens
        std::string clean;
        clean.reserve(str.size());
        for (char c : str) {
            if (c != '-') clean.push_back(c);
        }

        if (clean.size() != 32) {
            // Invalid length for a GUID string
            return;
        }

        for (size_t i = 0; i < 16; ++i) {
            int hi = hexCharToInt(clean[2 * i]);
            int lo = hexCharToInt(clean[2 * i + 1]);
            if (hi == -1 || lo == -1) {
                std::fill(std::begin(bytes), std::end(bytes), 0);
                return;
            }
            bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
        }
    }

    std::string GUID::ToString(bool withHyphens) const {
        static constexpr char hexChars[] = "0123456789abcdef";
        std::string str;
        str.reserve(withHyphens ? 36 : 32);

        for (size_t i = 0; i < 16; ++i) {
            if (withHyphens) {
                // Hyphen format: 8-4-4-4-12
                if (i == 4 || i == 6 || i == 8 || i == 10)
                    str.push_back('-');
            }
            uint8_t b = bytes[i];
            str.push_back(hexChars[(b >> 4) & 0xF]);
            str.push_back(hexChars[b & 0xF]);
        }

        return str;
    }
	

}