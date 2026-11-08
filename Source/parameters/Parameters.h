#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace psylance
{

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using PID = juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APF> (PID { "gain", 1 }, "Gain",
                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    layout.add (std::make_unique<APF> (PID { "cutoff", 1 }, "Cutoff",
                                        juce::NormalisableRange<float> (20.0f, 16000.0f, 0.1f, 0.3f), 8000.0f));
    layout.add (std::make_unique<APF> (PID { "resonance", 1 }, "Resonance",
                                        juce::NormalisableRange<float> (0.5f, 16.0f, 0.01f), 1.2f));
    layout.add (std::make_unique<APF> (PID { "envAmount", 1 }, "Filter Env",
                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<APF> (PID { "attack", 1 }, "Attack",
                                        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0001f, 0.3f), 0.005f));
    layout.add (std::make_unique<APF> (PID { "decay", 1 }, "Decay",
                                        juce::NormalisableRange<float> (0.01f, 3.0f, 0.001f, 0.3f), 0.2f));
    layout.add (std::make_unique<APF> (PID { "sustain", 1 }, "Sustain",
                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    layout.add (std::make_unique<APF> (PID { "release", 1 }, "Release",
                                        juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.3f), 0.3f));
    layout.add (std::make_unique<APF> (PID { "subLevel", 1 }, "Sub Level",
                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    return layout;
}

} // namespace psylance
