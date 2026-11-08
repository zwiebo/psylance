#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GnarlAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit GnarlAudioProcessorEditor(GnarlAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GnarlAudioProcessor& processorRef;

    juce::ComboBox presetBox;
    juce::TextButton mutateButton{ "Mutate Sound" };
    juce::Slider ratioSlider, indexSlider, fmDecaySlider, attackSlider,
        decaySlider, sustainSlider, releaseSlider, cutoffSlider,
        resonanceSlider, driveSlider, glideSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAtt, indexAtt,
        fmDecayAtt, attackAtt, decayAtt, sustainAtt, releaseAtt, cutoffAtt,
        resonanceAtt, driveAtt, glideAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnarlAudioProcessorEditor)
};
