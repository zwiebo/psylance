#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace psylance
{

enum class Waveform
{
    Saw,
    Square,
    Sine
};

class Oscillator
{
public:
    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    }

    void setFrequency (float newFrequency)
    {
        frequency = juce::jlimit (0.0f, (float) (sampleRate * 0.49), newFrequency);
    }

    void setWaveform (Waveform newWaveform) { waveform = newWaveform; }
    void setPulseWidth (float newPulseWidth) { pulseWidth = juce::jlimit (0.05f, 0.95f, newPulseWidth); }

    void reset() { phase = 0.0; }

    float process()
    {
        float sample = 0.0f;

        switch (waveform)
        {
            case Waveform::Saw:
                sample = (float) (2.0 * phase - 1.0);
                break;
            case Waveform::Square:
                sample = phase < pulseWidth ? 1.0f : -1.0f;
                break;
            case Waveform::Sine:
                sample = (float) std::sin (juce::MathConstants<double>::twoPi * phase);
                break;
        }

        phase += (double) frequency / sampleRate;
        if (phase >= 1.0)
            phase -= std::floor (phase);

        return sample;
    }

private:
    double phase = 0.0;
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float pulseWidth = 0.5f;
    Waveform waveform = Waveform::Saw;
};

} // namespace psylance
