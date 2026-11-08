#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace psylance
{

class StateVariableFilter
{
public:
    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updateCoefficients();
    }

    void setCutoff (float cutoffHz)
    {
        cutoff = juce::jlimit (10.0f, (float) (sampleRate * 0.49), cutoffHz);
        updateCoefficients();
    }

    void setResonance (float q)
    {
        resonance = juce::jlimit (0.5f, 20.0f, q);
        k = 1.0 / resonance;
    }

    void reset()
    {
        ic1eq = 0.0;
        ic2eq = 0.0;
    }

    float process (float input)
    {
        const double v0 = input;
        const double v3 = v0 - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        return (float) v2;
    }

private:
    void updateCoefficients()
    {
        const double g = std::tan (juce::MathConstants<double>::pi * (double) cutoff / sampleRate);
        k = 1.0 / resonance;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a1;
    }

    double sampleRate = 44100.0;
    float cutoff = 8000.0f;
    float resonance = 1.0f;
    double k = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    double ic1eq = 0.0;
    double ic2eq = 0.0;
};

} // namespace psylance
