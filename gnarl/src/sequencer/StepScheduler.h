#pragma once

#include <cstdint>
#include <vector>
#include "PatternGenerator.h"

namespace gnarl::sequencer {

// A note event positioned in host time (absolute sample index).
struct TimedEvent {
    int64_t samplePosition = 0;
    bool noteOn = false;   // false = note-off
    int note = 0;          // MIDI note number
    float velocity = 0.0f;
};

// Sample-accurate 16-step scheduler. Each step is a 16th note (0.25 beats).
// Tempo changes take effect at the next step boundary. stop() always emits a
// note-off for any sounding note: no stuck notes by construction.
class StepScheduler {
public:
    static constexpr int kNumSteps = 16;

    void setPattern(const Pattern& p) { pattern = p; }
    void setSampleRate(double sr) { sampleRate = sr > 0.0 ? sr : 44100.0; }
    void setTempo(double bpm) { tempoBpm = bpm > 1.0 ? bpm : 120.0; }

    // Begins playback at the start of the next process() call, from step 0.
    void start();
    // Stops playback; returns a note-off (at relative position 0) if a note
    // is sounding.
    std::vector<TimedEvent> stop();
    bool isPlaying() const { return playing; }
    int getCurrentStep() const { return stepIndex; }

    // Advances numSamples of host time. Returned events carry positions
    // relative to the start of this call (0..numSamples-1), sorted by position.
    std::vector<TimedEvent> process(int numSamples);

private:
    double stepDurationSamples() const;
    void enterStep(int64_t atSample, std::vector<TimedEvent>& out);

    Pattern pattern{};
    double sampleRate = 44100.0;
    double tempoBpm = 150.0;
    int64_t totalSamples = 0;     // absolute samples consumed so far
    int64_t nextStepSample = 0;   // absolute sample index of the next step boundary
    int stepIndex = 0;
    int soundingNote = -1;        // currently sounding note, -1 = none
    bool prevGlide = false;       // previous step tied into this one
    bool playing = false;
};

} // namespace gnarl::sequencer
