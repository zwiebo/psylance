#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Voice.h"

namespace psylance
{

class SynthEngine
{
public:
    static constexpr int maxVoices = 8;

    void setSampleRate (double newSampleRate);
    void setParams (const VoiceParams& newParams);

    void noteOn (int noteNumber, float velocity);
    void noteOff (int noteNumber);
    void allNotesOff();

    void handleMidi (const juce::MidiMessage& message);

    void renderBlock (float* output, int numSamples);

    int activeVoiceCount() const;

    void setPitchBendRange (double semitones) { pitchBendRange = semitones; }

private:
    Voice* findFreeVoice();

    std::array<Voice, maxVoices> voices;
    double sampleRate = 44100.0;
    double pitchBendRange = 2.0;
    double currentBend = 0.0;
    VoiceParams params;
    int stealIndex = 0;
};

} // namespace psylance
