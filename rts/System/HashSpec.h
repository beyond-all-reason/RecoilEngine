#pragma once

#include <tuple>
#include <concepts>
#include "SpringHash.h"

namespace spring {
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <typename T, std::unsigned_integral S>
        inline void hash_combine(S& seed, const T& v)
        {
            seed ^= spring::LiteHash(v) + S(0x9e3779b9) + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <typename Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
            template <std::unsigned_integral S>
            static void apply(S& seed, const Tuple& tuple)
            {
                HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
                hash_combine(seed, std::get<Index>(tuple));
            }
        };

        template <typename Tuple>
        struct HashValueImpl<Tuple,0>
        {
            template <std::unsigned_integral S>
            static void apply(S& seed, const Tuple& tuple)
            {
                hash_combine(seed, std::get<0>(tuple));
            }
        };
    }

    template <typename T, std::unsigned_integral S>
    inline S hash_combine(const T& v, S seed = S(1337))
    {
        seed ^= spring::LiteHash(v) + S(0x9e3779b9) + (seed << 6) + (seed >> 2);
        return seed;
    }
    template <typename T, std::unsigned_integral S>
    inline S hash_combine(S hashValue, S seed = S(1337))
    {
        seed ^= hashValue + S(0x9e3779b9) + (seed << 6) + (seed >> 2);
        return seed;
    }
}

// for std::tuple as a key in std::unordered_map / std::unordered_set
template <typename ... TT>
struct std::hash<std::tuple<TT...>>
{
    uint64_t operator()(std::tuple<TT...> const& tt) const
    {
        uint64_t seed = 0;
        spring::HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
        return seed;
    }
};