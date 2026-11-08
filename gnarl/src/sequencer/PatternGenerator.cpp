#include "PatternGenerator.h"

#include <algorithm>
#include <random>

namespace gnarl::sequencer {

namespace {

// Scale tables as semitone offsets from the root. Darkpsy-friendly minor
// flavors first; index is GeneratorSettings::scale.
const int kScales[][12] = {
    { 0, 2, 3, 5, 7, 8, 10 },              // 0: natural minor
    { 0, 2, 3, 5, 7, 8, 11 },              // 1: harmonic minor
    { 0, 1, 4, 5, 7, 8, 10 },              // 2: phrygian dominant
    { 0, 2, 4, 5, 7, 9, 10 },              // 3: dorian
    { 0, 1, 3, 5, 6, 8, 10 },              // 4: double harmonic
};
constexpr int kNumScales = 5;
constexpr int kScaleLen = 7;

} // namespace

Pattern generatePattern(const GeneratorSettings& s) {
    Pattern pattern;
    std::mt19937 rng(s.seed);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    const int scaleIdx = std::clamp(s.scale, 0, kNumScales - 1);
    const auto* scale = kScales[scaleIdx];
    const int octaves = std::clamp(s.octaveRange, 0, 3);
    const float density = std::clamp(s.density, 0.0f, 1.0f);

    for (auto& step : pattern.steps) {
        step.rest = uni(rng) >= density;
        if (step.rest)
            continue;

        step.noteOn = true;
        const int degree = static_cast<int>(uni(rng) * (kScaleLen * (octaves + 1)));
        const int octave = degree / kScaleLen;
        step.note = s.rootNote + 12 * octave + scale[degree % kScaleLen];

        const float velSpread = std::clamp(s.velocityVariation, 0.0f, 1.0f);
        step.velocity = std::clamp(1.0f - velSpread * uni(rng), 0.05f, 1.0f);
        step.accent = uni(rng) < std::clamp(s.accentProbability, 0.0f, 1.0f);
        step.glide = uni(rng) < std::clamp(s.glideProbability, 0.0f, 1.0f);
        step.length = step.glide ? 2.0f : 1.0f;
    }
    return pattern;
}

} // namespace gnarl::sequencer
