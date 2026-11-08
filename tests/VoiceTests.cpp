#include <juce_core/juce_core.h>
#include "dsp/Voice.h"

using namespace psylance;

namespace
{

float maxAbs (const std::vector<float>& v, size_t start)
{
    float peak = 0.0f;
    for (size_t i = start; i < v.size(); ++i)
        peak = std::max (peak, std::abs (v[i]));
    return peak;
}

class VoiceTests : public juce::UnitTest
{
public:
    VoiceTests() : juce::UnitTest ("Voice", "Psylance DSP") {}

    void runTest() override
    {
        constexpr double sampleRate = 44100.0;

        beginTest ("Note on renders audio, note off eventually silences");
        {
            Voice voice;
            voice.setSampleRate (sampleRate);
            voice.setParams (VoiceParams{});

            std::vector<float> buffer (4410, 0.0f);
            voice.noteOn (69, 1.0f, 0.0);
            voice.render (buffer.data(), (int) buffer.size());
            expect (maxAbs (buffer, 0) > 0.01f, "voice should produce audible output after note on");
            expect (voice.isActive(), "voice should be active while envelope runs");

            voice.noteOff();
            std::vector<float> tail (132300, 0.0f);
            voice.render (tail.data(), (int) tail.size());
            expect (maxAbs (tail, tail.size() - 4410) < 0.001f, "voice should be silent at the end of release");
            expect (! voice.isActive(), "voice should become inactive after release completes");
        }

        beginTest ("Silent before any note on");
        {
            Voice voice;
            voice.setSampleRate (sampleRate);
            std::vector<float> buffer (512, 0.0f);
            voice.render (buffer.data(), (int) buffer.size());
            expectEquals (maxAbs (buffer, 0), 0.0f, "inactive voice must render silence");
        }
    }
};

VoiceTests voiceTests;

} // namespace
