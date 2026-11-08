#pragma once

#include <array>
#include <cstdint>

namespace gnarl::sequencer {

// Host-independent pattern data: 16 steps, pure data, no audio or MIDI types.
struct StepEvent {
    bool noteOn = false;
    bool glide = false;   // tie into the next step
    bool accent = false;
    bool rest = true;
    int note = 0;         // MIDI note number
    float velocity = 0.0f;
    float length = 1.0f;  // in steps
};

struct Pattern {
    std::array<StepEvent, 16> steps{};
};

struct GeneratorSettings {
    int rootNote = 36;
    int scale = 0;
    int octaveRange = 1;
    float density = 0.7f;
    float velocityVariation = 0.3f;
    float accentProbability = 0.25f;
    float glideProbability = 0.2f;
    uint32_t seed = 1;
};

// Deterministic: same (settings) always yields the same pattern.
Pattern generatePattern(const GeneratorSettings& settings);

} // namespace gnarl::sequencer
