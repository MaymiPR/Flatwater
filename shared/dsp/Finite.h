#pragma once
// Input that is not a number or is infinite: taken as silence, sample by sample, before any DSP reads it. One such
// sample would otherwise stay in every recursive filter and learned state until the next prepare. Plain C++; reads
// the bits, so it does not depend on the compiler's floating-point mode.
#include <cstdint>
#include <cstring>

namespace flatwater
{
inline void silenceNonFinite (float* const* ch, int channels, int n) noexcept
{
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < n; ++i)
        {
            std::uint32_t bits;
            std::memcpy (&bits, &ch[c][i], sizeof bits);
            if ((bits & 0x7f800000u) == 0x7f800000u) ch[c][i] = 0.0f;
        }
}
}
