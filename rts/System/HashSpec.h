#pragma once

#include <tuple>
#include <concepts>

namespace spring {
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <typename T, std::unsigned_integral S>
        inline void hash_combine(S& seed, T const& v)
        {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <typename Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
            template <std::unsigned_integral S>
            static void apply(S& seed, Tuple const& tuple)
            {
                HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
                hash_combine(seed, std::get<Index>(tuple));
            }
        };

        template <typename Tuple>
        struct HashValueImpl<Tuple,0>
        {
            template <std::unsigned_integral S>
            static void apply(S& seed, Tuple const& tuple)
            {
                hash_combine(seed, std::get<0>(tuple));
            }
        };
    }

    template <typename T, std::unsigned_integral S>
    inline S hash_combine(T const& v, S seed = 1337u)
    {
        seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
    template <typename T, std::unsigned_integral S>
    inline S hash_combine(S hashValue, S seed = 1337u)
    {
        seed ^= hashValue + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
}

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