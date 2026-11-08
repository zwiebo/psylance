#pragma once

#include "Voice.h"

namespace gnarl::dsp {

// Monophonic synth engine: owns the single lead voice and renders blocks.
// Pure C++, no JUCE — unit-testable without a host.
class SynthEngine {
public:
    void setSampleRate(double sr) { voice.setSampleRate(sr); sampleRate = sr; }

    void noteOn(int midiNote, float velocity) { voice.noteOn(midiNote, velocity); }
    void noteOff(int midiNote) {
        if (voice.getCurrentNote() == midiNote || midiNote < 0)
            voice.noteOff();
    }
    void allNotesOff() { voice.allNotesOff(); }

    bool isActive() const { return voice.isActive(); }

    // Adds mono output into out[0..numSamples).
    void render(float* out, int numSamples, const VoiceParams& p) {
        voice.render(out, numSamples, p);
    }

private:
    Voice voice;
    double sampleRate = 44100.0;
};

} // namespace gnarl::dsp
