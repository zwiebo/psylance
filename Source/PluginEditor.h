#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class PsylanceAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PsylanceAudioProcessorEditor (PsylanceAudioProcessor& processor);
    ~PsylanceAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct ParamControl
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    PsylanceAudioProcessor& processorRef;
    std::vector<std::unique_ptr<ParamControl>> controls;

    void addControl (const juce::String& paramId, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PsylanceAudioProcessorEditor)
};
