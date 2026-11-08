#include "PatternGenerator.h"

#include <algorithm>

namespace gnarl::sequencer {

namespace {

// SplitMix64: small, fully specified PRNG. Unlike std::mt19937 (whose
// distributions are not portable across standard library implementations),
// it produces bit-identical sequences everywhere — that is what makes pattern
// generation deterministic across reruns and machines.
class SplitMix64 {
public:
    explicit SplitMix64(uint64_t seed) : m_state(seed) {}
    uint64_t next() {
        uint64_t z = (m_state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    double nextDouble() {  // [0, 1)
        return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0);
    }
    int nextInt(int n) {
        if (n <= 0) return 0;
        return static_cast<int>(next() % static_cast<uint64_t>(n));
    }
private:
    uint64_t m_state;
};

uint64_t stepSeed(uint32_t seed, int index) {
    SplitMix64 h(static_cast<uint64_t>(seed) ^
                 (static_cast<uint64_t>(index + 1) * 0xD1B54A32D192ED03ull));
    return h.next();
}

int wrapIndex(int index) {
    int i = index % Pattern::kNumSteps;
    if (i < 0) i += Pattern::kNumSteps;
    return i;
}

} // namespace

int numScales() { return kNumScales; }

const char* scaleName(int scale) {
    static const char* names[] = {"Phrygian Dominant", "Double Harmonic", "Harmonic Minor",
                                  "Phrygian", "Minor (natural)", "Major", "Chromatic"};
    if (scale < 0 || scale >= kNumScales) return "Unknown";
    return names[scale];
}

const std::vector<int>& scaleIntervals(int scale) {
    static const std::vector<int> phrygianDominant{0, 1, 4, 5, 7, 8, 10};
    static const std::vector<int> doubleHarmonic{0, 1, 4, 5, 7, 8, 11};
    static const std::vector<int> harmonicMinor{0, 2, 3, 5, 7, 8, 11};
    static const std::vector<int> phrygian{0, 1, 3, 5, 7, 8, 10};
    static const std::vector<int> minorNatural{0, 2, 3, 5, 7, 8, 10};
    static const std::vector<int> major{0, 2, 4, 5, 7, 9, 11};
    static const std::vector<int> chromatic{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    switch (scale) {
        case ScalePhrygianDominant: return phrygianDominant;
        case ScaleDoubleHarmonic: return doubleHarmonic;
        case ScaleHarmonicMinor: return harmonicMinor;
        case ScalePhrygian: return phrygian;
        case ScaleMinorNatural: return minorNatural;
        case ScaleMajor: return major;
        default: return chromatic;
    }
}

std::vector<int> scaleNotesInRange(int rootNote, int scale, int pitchMin, int pitchMax) {
    std::vector<int> out;
    const auto& intervals = scaleIntervals(scale);
    for (int octave = -2; octave <= 9; ++octave) {
        for (int iv : intervals) {
            int note = rootNote + 12 * octave + iv;
            if (note >= pitchMin && note <= pitchMax) out.push_back(note);
        }
    }
    return out;
}

bool isNoteInScale(int note, int rootNote, int scale) {
    int rel = (note - rootNote) % 12;
    if (rel < 0) rel += 12;
    const auto& intervals = scaleIntervals(scale);
    return std::find(intervals.begin(), intervals.end(), rel) != intervals.end();
}

std::string noteName(int midiNote) {
    static const char* names[] = {"C", "C#", "D", "D#", "E", "F",
                                  "F#", "G", "G#", "A", "A#", "B"};
    return std::string(names[midiNote % 12]) + std::to_string(midiNote / 12 - 1);
}

Pattern generatePattern(const GeneratorSettings& settings) {
    Pattern p;
    regeneratePattern(p, settings);
    return p;
}

void regeneratePattern(Pattern& pattern, const GeneratorSettings& settings) {
    const int pitchMin = std::clamp(settings.pitchMin, 0, 127);
    const int pitchMax = std::clamp(settings.pitchMax, pitchMin, 127);
    auto candidates = scaleNotesInRange(settings.rootNote, settings.scale, pitchMin, pitchMax);
    if (candidates.empty()) {
        candidates.push_back(std::clamp(settings.rootNote, pitchMin, pitchMax));
    }
    const float baseVelocity = 0.8f;
    for (int i = 0; i < Pattern::kNumSteps; ++i) {
        StepEvent& slot = pattern.steps[wrapIndex(i)];
        if (slot.locked) continue;
        SplitMix64 rng(stepSeed(settings.seed, i));
        StepEvent s;
        s.locked = false;
        s.noteOn = rng.nextDouble() < settings.density;
        s.rest = !s.noteOn;
        if (s.noteOn) {
            s.note = candidates[rng.nextInt(static_cast<int>(candidates.size()))];
            float v = baseVelocity + static_cast<float>(rng.nextDouble() * 2.0 - 1.0) *
                                            settings.velocityVariation * 0.5f;
            s.velocity = std::clamp(v, 0.05f, 1.0f);
            s.accent = rng.nextDouble() < settings.accentProbability;
            if (s.accent) s.velocity = 1.0f;
            s.glide = rng.nextDouble() < settings.glideProbability;
            s.length = 1.0f;
        }
        slot = s;
    }
}

int glideRunLength(const Pattern& pattern, int stepIndex) {
    const int start = wrapIndex(stepIndex);
    int len = 1;
    for (int k = 1; k < Pattern::kNumSteps; ++k) {
        const StepEvent& prev = pattern.steps[wrapIndex(start + k - 1)];
        const StepEvent& cur = pattern.steps[wrapIndex(start + k)];
        if (prev.noteOn && !prev.rest && prev.glide && cur.noteOn && !cur.rest) {
            ++len;
        } else {
            break;
        }
    }
    return len;
}

float noteLengthSteps(const Pattern& pattern, int stepIndex) {
    const StepEvent& s = pattern.steps[wrapIndex(stepIndex)];
    return std::max(s.length, static_cast<float>(glideRunLength(pattern, stepIndex)));
}

} // namespace gnarl::sequencer
