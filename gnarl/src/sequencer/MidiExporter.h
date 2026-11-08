#pragma once

#include <vector>
#include "PatternGenerator.h"

namespace gnarl::sequencer {

// M4 milestone: export a Pattern to a Standard MIDI File (format 0).
std::vector<uint8_t> exportMidiFile(const Pattern& pattern, double bpm);

} // namespace gnarl::sequencer
