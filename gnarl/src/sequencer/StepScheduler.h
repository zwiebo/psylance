#pragma once

#include "PatternGenerator.h"

namespace gnarl::sequencer {

// Converts a Pattern into host-timed note on/off events.
// M3 milestone: tempo-synchronized playback of a generated pattern.
class StepScheduler {
public:
    void setPattern(const Pattern& p) { pattern = p; }
    void setTempo(double bpm) { tempoBpm = bpm; }
    void start();
    void stop();

private:
    Pattern pattern{};
    double tempoBpm = 150.0;
};

} // namespace gnarl::sequencer
