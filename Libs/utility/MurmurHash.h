#pragma once

#include <cstdint>

namespace Util
{

#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

static FORCE_INLINE uint32_t rotl32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

#define ROTL32(x, y) rotl32(x, y)

#define BIG_CONSTANT(x) (x##LLU)

#define getblock(p, i) (p[i])

/**
 * @brief A 32bit mumur3 hasher adapted from here: https://github.com/PeterScott/murmur3
 * This is for the use with smaller keys with an improvement in performance compared to 128bit
 * versions
 */
uint32_t murmurHash3(const void* key, size_t len, uint32_t seed);

// for use with standard lib maps as use as a hasher
template <typename T>
struct Murmur3Hasher
{
    uint32_t operator()(const T& key) const noexcept
    {
        static_assert(sizeof(key) % 4 == 0, "The key length must be a multiple of four");
        return murmurHash3((const void*) &key, sizeof(key), 0);
    }
};

} // namespace Util