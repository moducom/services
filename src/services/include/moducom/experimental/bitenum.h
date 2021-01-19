#pragma once

#include <bitset>

namespace moducom { namespace experimental {

/// Space efficient manager of one or many enum values
/// \tparam TEnum
/// \tparam N
template <typename TEnum, std::size_t N = sizeof(int) * 8>
struct BitEnum
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

    BitEnum() : enum_(0)
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

    BitEnum& operator=(TEnum e)
    {
        enum_ = e + 1;
    }
};

}}