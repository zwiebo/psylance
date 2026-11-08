#pragma once

#include <random>
#include "FMSynth.h"

namespace gnarl::dsp {

// Biased randomization toward useful darkpsy timbres (Mutate Sound).
// Pure function of (params, rng) — deterministic for a given rng state.
void mutateSound(VoiceParams& p, std::mt19937& rng);

} // namespace gnarl::dsp
