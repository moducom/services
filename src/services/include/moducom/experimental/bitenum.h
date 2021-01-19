#pragma once

#include <bitset>
#include <type_traits>

namespace moducom { namespace experimental {

/// Space efficient manager of one or many enum values
/// \tparam TEnum
/// \tparam N
template <typename TEnum, std::size_t N = sizeof(int) * 8>
struct BitEnumOld
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    static constexpr int msb = N - 1;
#else
    static constexpr int msb = 0;
#endif

    typedef unsigned uint_type;

    union
    {
        std::bitset<N> bits_;
        // Stored as +1 regular enum so that we can tell if an enum has been assigned here
        // (nonzero means enum_ is used, providing bits_ msb is clear)
        uint_type enum_;
    };

    BitEnumOld() : enum_(0)
    {

    }

    bool in_bits(TEnum e) const
    {
        return(bits_[(uint_type)e]);
    }

    bool in_enum(TEnum e) const
    {
        return enum_ == (uint_type)e + 1;
    }

    bool multi() const { return bits_[msb]; }

    bool contains(TEnum e) const
    {
        return multi() ? in_bits(e) : in_enum(e);
    }

    void set(TEnum _e)
    {
        uint_type e = (uint_type) _e;

        if(multi())
            bits_[e] = true;
        else
        {
            if(enum_ == 0)
                enum_ = e + 1;
            else
            {
                // promote to bit mapped
                bits_[enum_ - 1] = true;
                bits_[msb] = true;
                bits_[e] = true;
            }
        }
    }

    BitEnumOld& operator=(TEnum e)
    {
        enum_ = e + 1;
    }
};

namespace internal {

/* FIX: Enable this -- right now doesn't work like this, says call is ambiguous
 * Trying to do SFINAE
template <typename TEnum>
constexpr std::size_t enum_max() { return sizeof(int) * 8; } */

template <typename TEnum, std::enable_if_t<((int)TEnum::Max != 0), bool> = true>
constexpr std::size_t enum_max() { return (std::size_t)TEnum::Max; }




}

template <typename TEnum, std::size_t _N = 0>
class BitEnum
{
    static constexpr std::size_t N =
        _N == 0 ? internal::enum_max<TEnum>() : _N;

    std::bitset<N> bits_;

    typedef unsigned uint_type;

    void set() const {}

public:
    template <class ...TArgs>
    void set(TEnum e, TArgs&&...args)
    {
        bits_[(uint_type)e] = true;
        set(std::forward<TArgs>(args)...);
    }

    template <class ...TArgs>
    BitEnum(TArgs&&...args)
    {
        set(std::forward<TArgs>(args)...);
    }

    bool contains(TEnum e) const
    {
        return bits_[(uint_type)e];
    }
};

}}