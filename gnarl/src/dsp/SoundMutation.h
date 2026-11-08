#pragma once

#include <cstdint>
#include <random>
#include "FMSynth.h"
#include "Patch.h"

namespace gnarl::dsp {

// Biased randomization toward useful darkpsy timbres (Mutate Sound).
// Pure function of (params, rng) — deterministic for a given rng state.
void mutateSound(VoiceParams& p, std::mt19937& rng);

// Seeded variant: pure function of the seed alone, no shared rng state.
inline VoiceParams mutateSoundSeeded(uint32_t seed) { return randomizeParams(seed); }

} // namespace gnarl::dsp
