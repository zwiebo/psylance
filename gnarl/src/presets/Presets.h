#pragma once

#include "dsp/FMSynth.h"

namespace gnarl::presets {

int count();
const char* name(int index);
// Returns the factory sound for a preset index (0..count()-1).
gnarl::dsp::VoiceParams factory(int index);

} // namespace gnarl::presets
