#include <juce_core/juce_core.h>
#include "dsp/StateVariableFilter.h"

using namespace psylance;

namespace
{

float rmsOfSine (double freq, double sampleRate, int numSamples, int skip)
{
    double sum = 0.0;
    int count = 0;
    for (int n = 0; n < numSamples; ++n)
    {
        const float s = (float) std::sin (juce::MathConstants<double>::twoPi * freq * n / sampleRate);
        if (n >= skip)
        {
            sum += (double) s * s;
            ++count;
        }
    }
    return (float) std::sqrt (sum / count);
}

float renderSineThroughFilter (double freq, double cutoff, float resonance, double sampleRate)
{
    StateVariableFilter filter;
    filter.setSampleRate (sampleRate);
    filter.setCutoff ((float) cutoff);
    filter.setResonance (resonance);

    double sum = 0.0;
    int count = 0;
    constexpr int numSamples = 44100;
    for (int n = 0; n < numSamples; ++n)
    {
        const float in = (float) std::sin (juce::MathConstants<double>::twoPi * freq * n / sampleRate);
        const float out = filter.process (in);
        if (n >= numSamples / 2)
        {
            sum += (double) out * out;
            ++count;
        }
    }
    return (float) std::sqrt (sum / count);
}

class FilterTests : public juce::UnitTest
{
public:
    FilterTests() : juce::UnitTest ("State Variable Filter", "Psylance DSP") {}

    void runTest() override
    {
        constexpr double sampleRate = 44100.0;

        beginTest ("Unity gain at DC");
        {
            StateVariableFilter filter;
            filter.setSampleRate (sampleRate);
            filter.setCutoff (1000.0f);
            filter.setResonance (1.0f);

            float out = 0.0f;
            for (int n = 0; n < 5000; ++n)
                out = filter.process (1.0f);
            expectWithinAbsoluteError (out, 1.0f, 0.01f, "lowpass must pass DC with unity gain, got " + juce::String (out));
        }

        beginTest ("Passband sine is preserved");
        {
            const float inRms = rmsOfSine (100.0, sampleRate, 44100, 22050);
            const float outRms = renderSineThroughFilter (100.0, 2000.0, 1.0f, sampleRate);
            expect (outRms > inRms * 0.9f && outRms < inRms * 1.1f,
                    "100 Hz sine with 2 kHz cutoff should pass nearly unchanged");
        }

        beginTest ("Stopband sine is attenuated");
        {
            const float inRms = rmsOfSine (10000.0, sampleRate, 44100, 22050);
            const float outRms = renderSineThroughFilter (10000.0, 500.0, 1.0f, sampleRate);
            expect (outRms < inRms * 0.3f,
                    "10 kHz sine with 500 Hz cutoff should be strongly attenuated, ratio "
                    + juce::String (outRms / inRms));
        }

        beginTest ("High resonance stays stable");
        {
            StateVariableFilter filter;
            filter.setSampleRate (sampleRate);
            filter.setCutoff (3000.0f);
            filter.setResonance (16.0f);

            juce::Random rng (42);
            bool finite = true;
            float peak = 0.0f;
            for (int n = 0; n < 20000; ++n)
            {
                const float out = filter.process (rng.nextFloat() * 2.0f - 1.0f);
                if (! std::isfinite (out))
                    finite = false;
                peak = std::max (peak, std::abs (out));
            }
            expect (finite, "filter output must stay finite at high resonance");
            expect (peak < 100.0f, "filter output must stay bounded at high resonance, peak " + juce::String (peak));
        }
    }
};

FilterTests filterTests;

} // namespace
