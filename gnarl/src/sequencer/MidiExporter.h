#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "PatternGenerator.h"

namespace gnarl::sequencer {

// A note in the tick domain of a Standard MIDI File.
struct MidiNote {
    int startTick = 0;
    int endTick = 0;  // exclusive
    int note = 60;
    int velocity = 100;  // 1..127
};

// Converts a pattern into MIDI notes at PPQ resolution. Glide runs are merged
// into single longer notes (legato, no pitch bend) — this is the SMF meaning
// of glide, which intentionally differs from the synth's internal portamento.
std::vector<MidiNote> patternToMidiNotes(const Pattern& pattern, int ppq = 480);

// Builds a complete Standard MIDI File (format 0, single track, PPQ division)
// as raw bytes. Deterministic: same input -> identical bytes.
std::vector<uint8_t> buildMidiFile(const std::vector<MidiNote>& notes, double bpm,
                                   int ppq = 480);

// M4 milestone: export a Pattern to a Standard MIDI File (format 0).
std::vector<uint8_t> exportMidiFile(const Pattern& pattern, double bpm);

// Convenience: write the file to disk. Returns false on I/O error.
bool writeMidiFile(const std::string& path, const Pattern& pattern, double bpm,
                   int ppq = 480);

} // namespace gnarl::sequencer
