#include "Parameters.h"

namespace gnarl::params {

static juce::NormalisableRange<float> skewed(double lo, double hi, double skew, double step = 0.0) {
    if (step > 0.0)
        return { static_cast<float>(lo), static_cast<float>(hi), static_cast<float>(step) };
    return { static_cast<float>(lo), static_cast<float>(hi), static_cast<float>(skew) };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<P>(juce::ParameterID{ "ratio", 1 }, "Ratio",
        skewed(0.25, 16.0, 0.0, 0.01), 1.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "modIndex", 1 }, "FM Index",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 4.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "fmEnvDecay", 1 }, "FM Env Decay",
        skewed(0.005, 4.0, 0.35), 0.25f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "fmEnvAmount", 1 }, "FM Env Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "attack", 1 }, "Attack",
        skewed(0.001, 2.0, 0.3), 0.005f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "decay", 1 }, "Decay",
        skewed(0.005, 4.0, 0.35), 0.25f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "release", 1 }, "Release",
        skewed(0.005, 5.0, 0.35), 0.3f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "cutoff", 1 }, "Filter Cutoff",
        skewed(40.0, 18000.0, 0.3), 6000.0f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "resonance", 1 }, "Resonance",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.2f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "drive", 1 }, "Distortion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "glide", 1 }, "Glide",
        skewed(0.0, 1.0, 0.4), 0.08f));
    layout.add(std::make_unique<P>(juce::ParameterID{ "gain", 1 }, "Output Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

    return layout;
}

} // namespace gnarl::params
