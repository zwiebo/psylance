#pragma once

#include "FMSynth.h"

#include <cstdint>
#include <random>

namespace gnarl::dsp {

struct FactoryPatch {
    const char* name;
    VoiceParams params;
};

inline constexpr int kNumFactoryPatches = 5;

// Factory patches: Init plus the four darkpsy signature voices
// (bubbly hit, squelchy lead, dark growl, metallic stab).
inline FactoryPatch factoryPatch(int index) {
    VoiceParams p;
    const char* name = "Init";
    switch (index) {
        case 1:
            name = "Bubbly Hit";
            p.ratio = 2.0f; p.modIndex = 6.0f; p.fmEnvDecay = 0.12f; p.fmEnvAmount = 1.0f;
            p.attack = 0.003f; p.decay = 0.3f; p.sustain = 0.9f; p.release = 0.25f;
            p.cutoff = 9000.0f; p.resonance = 0.35f; p.drive = 0.2f; p.glide = 0.06f;
            break;
        case 2:
            name = "Squelchy Lead";
            p.ratio = 1.0f; p.modIndex = 7.0f; p.fmEnvDecay = 0.35f; p.fmEnvAmount = 0.9f;
            p.attack = 0.005f; p.decay = 0.4f; p.sustain = 0.75f; p.release = 0.35f;
            p.cutoff = 3500.0f; p.resonance = 0.7f; p.drive = 0.45f; p.glide = 0.09f;
            break;
        case 3:
            name = "Dark Growl";
            p.ratio = 1.0f; p.modIndex = 9.0f; p.fmEnvDecay = 0.5f; p.fmEnvAmount = 0.9f;
            p.attack = 0.01f; p.decay = 0.5f; p.sustain = 0.7f; p.release = 0.4f;
            p.cutoff = 1800.0f; p.resonance = 0.6f; p.drive = 0.7f; p.glide = 0.12f;
            break;
        case 4:
            name = "Metallic Stab";
            p.ratio = 4.0f; p.modIndex = 5.0f; p.fmEnvDecay = 0.8f; p.fmEnvAmount = 0.7f;
            p.attack = 0.002f; p.decay = 0.6f; p.sustain = 0.6f; p.release = 0.5f;
            p.cutoff = 12000.0f; p.resonance = 0.5f; p.drive = 0.6f; p.glide = 0.03f;
            break;
        default:
            break;
    }
    return { name, p };
}

// Seeded, deterministic, darkpsy-biased randomization. A pure function of
// the seed: no shared state, safe to call from any thread, identical results
// for identical seeds.
inline VoiceParams randomizeParams(uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    static const float ratios[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    std::uniform_int_distribution<int> ratioPick(0, 8);

    VoiceParams p;
    p.ratio = ratios[ratioPick(rng)];
    p.modIndex = 2.0f + 8.0f * uni(rng);
    p.fmEnvDecay = 0.05f + 0.6f * uni(rng);
    p.fmEnvAmount = 0.5f + 0.5f * uni(rng);
    p.attack = 0.001f + 0.05f * uni(rng);
    p.decay = 0.1f + 0.5f * uni(rng);
    p.sustain = 0.4f + 0.6f * uni(rng);
    p.release = 0.1f + 0.5f * uni(rng);
    p.cutoff = 800.0f + 13200.0f * std::pow(uni(rng), 0.5f);
    p.resonance = 0.1f + 0.6f * uni(rng);
    p.drive = 0.1f + 0.7f * uni(rng);
    p.glide = 0.25f * uni(rng);
    p.gain = 0.7f + 0.3f * uni(rng);
    p.legato = 1.0f;
    p.filterMode = 0;
    return p;
}

} // namespace gnarl::dsp
