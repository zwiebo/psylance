#include <juce_core/juce_core.h>
#include "dsp/Oscillator.h"

using namespace psylance;

namespace
{

class OscillatorTests : public juce::UnitTest
{
public:
    OscillatorTests() : juce::UnitTest ("Oscillator", "Psylance DSP") {}

    void runTest() override
    {
        constexpr double sampleRate = 44100.0;

        beginTest ("Sine matches reference");
        {
            Oscillator osc;
            osc.setSampleRate (sampleRate);
            osc.setWaveform (Waveform::Sine);
            osc.setFrequency (440.0f);

            float maxError = 0.0f;
            for (int n = 0; n < 44100; ++n)
            {
                const float expected = (float) std::sin (juce::MathConstants<double>::twoPi * 440.0 * n / sampleRate);
                maxError = std::max (maxError, std::abs (osc.process() - expected));
            }
            expect (maxError < 1e-4f, "sine output should match sin(2*pi*f*t), max error was " + juce::String (maxError));
        }

        beginTest ("Saw stays in range and has correct frequency");
        {
            Oscillator osc;
            osc.setSampleRate (sampleRate);
            osc.setWaveform (Waveform::Saw);
            osc.setFrequency (220.0f);

            int crossings = 0;
            float prev = osc.process();
            float peak = 0.0f;
            for (int n = 1; n < 44100; ++n)
            {
                const float s = osc.process();
                peak = std::max (peak, std::abs (s));
                if (prev < 0.0f && s >= 0.0f)
                    ++crossings;
                prev = s;
            }
            expect (peak <= 1.0f + 1e-6f, "saw amplitude must not exceed 1");
            expectWithinAbsoluteError (crossings, 220, 2, "220 Hz saw should cross zero upward ~220 times per second");
        }

        beginTest ("Square outputs only +/-1");
        {
            Oscillator osc;
            osc.setSampleRate (sampleRate);
            osc.setWaveform (Waveform::Square);
            osc.setFrequency (100.0f);

            bool ok = true;
            for (int n = 0; n < 5000; ++n)
            {
                const float s = osc.process();
                if (std::abs (std::abs (s) - 1.0f) > 1e-6f)
                    ok = false;
            }
            expect (ok, "square wave must only output +/-1");
        }
    }
};

OscillatorTests oscillatorTests;

} // namespace
