#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/SynthEngine.h"

using namespace psylance;

namespace
{

int countRisingZeroCrossings (const std::vector<float>& v)
{
    int crossings = 0;
    for (size_t i = 1; i < v.size(); ++i)
        if (v[i - 1] < 0.0f && v[i] >= 0.0f)
            ++crossings;
    return crossings;
}

class SynthEngineTests : public juce::UnitTest
{
public:
    SynthEngineTests() : juce::UnitTest ("Synth Engine", "Psylance DSP") {}

    void runTest() override
    {
        constexpr double sampleRate = 44100.0;

        beginTest ("MIDI note on/off produces and releases sound");
        {
            SynthEngine engine;
            engine.setSampleRate (sampleRate);
            engine.setParams (VoiceParams{});

            engine.handleMidi (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100));
            expectEquals (engine.activeVoiceCount(), 1, "one note on should activate one voice");

            std::vector<float> buffer (512, 0.0f);
            engine.renderBlock (buffer.data(), (int) buffer.size());
            float peak = 0.0f;
            for (float s : buffer)
                peak = std::max (peak, std::abs (s));
            expect (peak > 0.001f, "engine should render audio for an active note");

            engine.handleMidi (juce::MidiMessage::noteOff (1, 60));
            std::vector<float> tail (132300, 0.0f);
            engine.renderBlock (tail.data(), (int) tail.size());
            expectEquals (engine.activeVoiceCount(), 0, "all voices should retire after release");
        }

        beginTest ("Polyphony up to max voices with stealing beyond");
        {
            SynthEngine engine;
            engine.setSampleRate (sampleRate);
            engine.setParams (VoiceParams{});

            for (int note = 60; note < 60 + SynthEngine::maxVoices + 4; ++note)
                engine.handleMidi (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100));

            expect (engine.activeVoiceCount() <= SynthEngine::maxVoices,
                    "voice count must be capped at maxVoices");
        }

        beginTest ("Pitch bend raises frequency");
        {
            SynthEngine engine;
            engine.setSampleRate (sampleRate);
            engine.setParams (VoiceParams{});

            engine.handleMidi (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 127));
            std::vector<float> base (44100, 0.0f);
            engine.renderBlock (base.data(), (int) base.size());
            engine.handleMidi (juce::MidiMessage::noteOff (1, 69));
            std::vector<float> release (44100, 0.0f);
            engine.renderBlock (release.data(), (int) release.size());

            engine.handleMidi (juce::MidiMessage (0xE0, 0x7F, 0x7F));
            engine.handleMidi (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 127));
            std::vector<float> bent (44100, 0.0f);
            engine.renderBlock (bent.data(), (int) bent.size());

            const double ratio = (double) countRisingZeroCrossings (bent)
                               / (double) countRisingZeroCrossings (base);
            expectWithinAbsoluteError ((float) ratio, (float) std::pow (2.0, 2.0 / 12.0), 0.05f,
                                       "full pitch bend up should raise pitch by the bend range (2 semitones)");
        }
    }
};

SynthEngineTests synthEngineTests;

} // namespace
