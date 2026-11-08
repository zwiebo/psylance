#include <juce_core/juce_core.h>
#include "dsp/AdsrEnvelope.h"

using namespace psylance;

namespace
{

class EnvelopeTests : public juce::UnitTest
{
public:
    EnvelopeTests() : juce::UnitTest ("ADSR Envelope", "Psylance DSP") {}

    void runTest() override
    {
        constexpr double sampleRate = 44100.0;

        beginTest ("Attack reaches full level, decay settles at sustain");
        {
            AdsrEnvelope env;
            env.setSampleRate (sampleRate);
            env.setParameters (0.01f, 0.1f, 0.5f, 0.1f);

            env.noteOn();
            float level = 0.0f;
            for (int n = 0; n < 500; ++n)
                level = env.process();
            expectWithinAbsoluteError (level, 1.0f, 0.01f, "after attack time the level should be ~1");

            for (int n = 0; n < 44100; ++n)
                level = env.process();
            expectWithinAbsoluteError (level, 0.5f, 0.02f, "after long decay the level should approach sustain");
            expect (env.getStage() == AdsrEnvelope::Stage::Sustain, "envelope should reach sustain stage");
        }

        beginTest ("Release falls to zero and becomes idle");
        {
            AdsrEnvelope env;
            env.setSampleRate (sampleRate);
            env.setParameters (0.005f, 0.05f, 0.7f, 0.05f);

            env.noteOn();
            float level = 0.0f;
            for (int n = 0; n < 44100; ++n)
                level = env.process();

            env.noteOff();
            for (int n = 0; n < 44100; ++n)
                level = env.process();

            expect (level < 0.001f, "after release the level should be ~0, got " + juce::String (level));
            expect (! env.isActive(), "envelope should be idle after release completes");
        }

        beginTest ("Idle envelope outputs silence");
        {
            AdsrEnvelope env;
            env.setSampleRate (sampleRate);
            bool silent = true;
            for (int n = 0; n < 1000; ++n)
                if (env.process() != 0.0f)
                    silent = false;
            expect (silent, "idle envelope must output 0");
        }
    }
};

EnvelopeTests envelopeTests;

} // namespace
