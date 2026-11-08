#include "SoundMutation.h"

namespace gnarl::dsp {

void mutateSound(VoiceParams& p, std::mt19937& rng) {
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    static const float ratios[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    std::uniform_int_distribution<size_t> ratioPick(0, 8);
    std::uniform_real_distribution<float> indexDist(2.0f, 10.0f);
    std::uniform_real_distribution<float> cutoffDist(800.0f, 14000.0f);

    p.ratio = ratios[ratioPick(rng)];
    p.modIndex = indexDist(rng);
    p.fmEnvDecay = 0.05f + 0.6f * uni(rng);
    p.cutoff = cutoffDist(rng);
    p.drive = 0.1f + 0.7f * uni(rng);
    p.resonance = 0.1f + 0.6f * uni(rng);
    p.glide = 0.02f + 0.2f * uni(rng);
}

} // namespace gnarl::dsp
