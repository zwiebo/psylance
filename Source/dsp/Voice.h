#pragma once

#include "Pitch.h"
#include "Oscillator.h"
#include "StateVariableFilter.h"
#include "AdsrEnvelope.h"

namespace psylance
{

struct VoiceParams
{
    float cutoff = 8000.0f;
    float resonance = 1.0f;
    float filterEnvAmount = 0.6f;
    float attack = 0.005f;
    float decay = 0.2f;
    float sustain = 0.8f;
    float release = 0.3f;
    float subLevel = 0.3f;
};

class Voice
{
public:
    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        osc.setSampleRate (sampleRate);
        sub.setSampleRate (sampleRate);
        filter.setSampleRate (sampleRate);
        ampEnv.setSampleRate (sampleRate);
        filterEnv.setSampleRate (sampleRate);
    }

    void setParams (const VoiceParams& newParams)
    {
        params = newParams;
        filter.setResonance (params.resonance);
    }

    void noteOn (int noteNumber, float velocity, double bendSemitones)
    {
        note = noteNumber;
        vel = juce::jlimit (0.0f, 1.0f, velocity);
        bend = bendSemitones;

        const double freq = noteToFreq ((double) noteNumber + bend);
        osc.setWaveform (Waveform::Saw);
        osc.setFrequency ((float) freq);
        osc.reset();
        sub.setWaveform (Waveform::Sine);
        sub.setFrequency ((float) (freq * 0.5));
        sub.reset();

        ampEnv.setParameters (params.attack, params.decay, params.sustain, params.release);
        filterEnv.setParameters (params.attack, params.decay, params.sustain, params.release);
        ampEnv.noteOn();
        filterEnv.noteOn();

        active = true;
    }

    void noteOff()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    bool isActive() const { return active && ampEnv.isActive(); }
    int getNote() const { return note; }

    void render (float* output, int numSamples)
    {
        const float maxCutoff = (float) (sampleRate * 0.45);

        for (int i = 0; i < numSamples; ++i)
        {
            const float amp = ampEnv.process();
            const float env = filterEnv.process();

            float cutoff = params.cutoff * (1.0f + 4.0f * params.filterEnvAmount * env);
            filter.setCutoff (juce::jlimit (20.0f, maxCutoff, cutoff));

            float s = osc.process() + params.subLevel * sub.process();
            output[i] += filter.process (s) * amp * vel;
        }

        if (!ampEnv.isActive())
            active = false;
    }

private:
    Oscillator osc;
    Oscillator sub;
    StateVariableFilter filter;
    AdsrEnvelope ampEnv;
    AdsrEnvelope filterEnv;
    VoiceParams params;
    double sampleRate = 44100.0;
    int note = -1;
    float vel = 0.0f;
    double bend = 0.0;
    bool active = false;
};

} // namespace psylance
