#pragma once

#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace psylance
{

class AdsrEnvelope
{
public:
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updateRates();
    }

    void setParameters (float attackSeconds, float decaySeconds, float sustainLevel, float releaseSeconds)
    {
        attack  = juce::jlimit (0.0005f, 5.0f, attackSeconds);
        decay   = juce::jlimit (0.001f,  5.0f, decaySeconds);
        sustain = juce::jlimit (0.0f, 1.0f, sustainLevel);
        release = juce::jlimit (0.001f,  10.0f, releaseSeconds);
        updateRates();
    }

    void noteOn()
    {
        stage = Stage::Attack;
    }

    void noteOff()
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    bool isActive() const { return stage != Stage::Idle; }
    Stage getStage() const { return stage; }
    float getLevel() const { return level; }

    float process()
    {
        switch (stage)
        {
            case Stage::Attack:
                level += attackStep;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                }
                break;
            case Stage::Decay:
                level += (sustain - level) * decayCoef;
                if (std::abs (sustain - level) < 0.001f)
                {
                    level = sustain;
                    stage = Stage::Sustain;
                }
                break;
            case Stage::Sustain:
                level = sustain;
                break;
            case Stage::Release:
                level += (0.0f - level) * releaseCoef;
                if (level < 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
                break;
            case Stage::Idle:
                level = 0.0f;
                break;
        }

        return level;
    }

private:
    void updateRates()
    {
        attackStep  = (float) (1.0 / (attack * sampleRate));
        decayCoef   = 1.0f - std::exp (-1.0f / (float) (decay * sampleRate));
        releaseCoef = 1.0f - std::exp (-1.0f / (float) (release * sampleRate));
    }

    double sampleRate = 44100.0;
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.8f;
    float release = 0.2f;
    float attackStep = 0.0f;
    float decayCoef = 0.0f;
    float releaseCoef = 0.0f;
    float level = 0.0f;
    Stage stage = Stage::Idle;
};

} // namespace psylance
