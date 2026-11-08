#pragma once

#include <cmath>

namespace psylance
{

inline double noteToFreq (double midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69.0) / 12.0);
}

} // namespace psylance
