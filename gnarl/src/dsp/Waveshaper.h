#pragma once

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

// Soft clipper (tanh waveshaper). drive 0..1 maps to a pre-gain of 1..8,
// normalized so full-scale input still maps to +/-1: bounded output,
// no runaway gain, no clicks from hard clipping.
inline float softClip(float x, float drive) {
    const float d = std::clamp(drive, 0.0f, 1.0f);
    const float g = 1.0f + 7.0f * d;
    const float y = std::tanh(g * x) / std::tanh(g);
    return std::clamp(y, -1.0f, 1.0f);
}

} // namespace gnarl::dsp
