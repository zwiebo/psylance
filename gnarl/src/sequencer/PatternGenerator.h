#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace gnarl::sequencer {

// One 16th-note step of the pattern.
//
// `glide` is the tie/slide flag. Contract shared with the DSP side (the voice
// is mono with portamento): glide means LEGATO — the next active step is not
// re-articulated; this note sustains through the glide run and the covered
// step's pitch is the portamento target. In MIDI file export the run is one
// merged note with no pitch bend, so internal glide and SMF export are
// intentionally not identical. See docs/SEQUENCER.md.
struct StepEvent {
    bool noteOn = false;
    bool glide = false;   // tie into the next step
    bool accent = false;
    bool rest = true;
    bool locked = false;  // locked steps survive regeneratePattern()
    int note = 0;         // MIDI note number
    float velocity = 0.0f;  // 0..1
    float length = 1.0f;    // in steps
};

struct Pattern {
    static constexpr int kNumSteps = 16;  // one 4/4 bar of 16th notes
    std::array<StepEvent, kNumSteps> steps{};
};

// Indices for GeneratorSettings::scale (darkpsy-friendly order).
enum Scale {
    ScalePhrygianDominant = 0,
    ScaleDoubleHarmonic = 1,
    ScaleHarmonicMinor = 2,
    ScalePhrygian = 3,
    ScaleMinorNatural = 4,
    ScaleMajor = 5,
    ScaleChromatic = 6,
    kNumScales = 7
};

int numScales();
const char* scaleName(int scale);
const std::vector<int>& scaleIntervals(int scale);

struct GeneratorSettings {
    int rootNote = 36;          // C2
    int scale = ScalePhrygianDominant;
    int pitchMin = 33;          // A1
    int pitchMax = 60;          // C4
    float density = 0.7f;       // probability a step is a note (else a rest)
    float velocityVariation = 0.3f;  // 0..1, spread around the base velocity
    float accentProbability = 0.25f;
    float glideProbability = 0.2f;
    uint32_t seed = 1;
};

// All MIDI notes in `scale` rooted at `rootNote` within [pitchMin, pitchMax].
std::vector<int> scaleNotesInRange(int rootNote, int scale, int pitchMin, int pitchMax);
bool isNoteInScale(int note, int rootNote, int scale);
std::string noteName(int midiNote);  // 69 -> "A4"

// Deterministic: same (settings) always yields the same pattern. Every step
// derives its random stream from hash(seed, stepIndex), so results are stable
// across reruns and platforms, and per-step locks never perturb other steps.
Pattern generatePattern(const GeneratorSettings& settings);

// Regenerates every unlocked step in place; locked steps are kept as-is.
void regeneratePattern(Pattern& pattern, const GeneratorSettings& settings);

// Steps covered by the glide run starting at stepIndex (capped at kNumSteps).
int glideRunLength(const Pattern& pattern, int stepIndex);

// Note length in steps: the glide run, extended by an explicit length > run.
float noteLengthSteps(const Pattern& pattern, int stepIndex);

} // namespace gnarl::sequencer
