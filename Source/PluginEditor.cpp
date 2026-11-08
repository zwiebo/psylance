#include "PluginEditor.h"

PsylanceAudioProcessorEditor::PsylanceAudioProcessorEditor (PsylanceAudioProcessor& processor)
    : juce::AudioProcessorEditor (&processor),
      processorRef (processor)
{
    addControl ("gain", "Gain");
    addControl ("cutoff", "Cutoff");
    addControl ("resonance", "Resonance");
    addControl ("envAmount", "Filter Env");
    addControl ("attack", "Attack");
    addControl ("decay", "Decay");
    addControl ("sustain", "Sustain");
    addControl ("release", "Release");
    addControl ("subLevel", "Sub Level");

    setSize (720, 320);
}

PsylanceAudioProcessorEditor::~PsylanceAudioProcessorEditor() = default;

void PsylanceAudioProcessorEditor::addControl (const juce::String& paramId, const juce::String& name)
{
    auto control = std::make_unique<ParamControl>();
    control->slider.setSliderStyle (juce::Slider::LinearVertical);
    control->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    control->slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffa04ae3));
    control->label.setText (name, juce::dontSendNotification);
    control->label.setJustificationType (juce::Justification::centred);
    control->label.setColour (juce::Label::textColourId, juce::Colours::white);
    control->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, paramId, control->slider);

    addAndMakeVisible (control->slider);
    addAndMakeVisible (control->label);
    controls.push_back (std::move (control));
}

void PsylanceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14101c));
    g.setColour (juce::Colour (0xffa04ae3));
    g.setFont (juce::FontOptions (22.0f));
    g.drawText ("PSYLANCE", 12, 6, 200, 26, juce::Justification::left);
}

void PsylanceAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (34);

    const int numControls = (int) controls.size();
    const int columnWidth = area.getWidth() / numControls;

    for (auto& control : controls)
    {
        auto column = area.removeFromLeft (columnWidth);
        control->label.setBounds (column.removeFromBottom (20));
        control->slider.setBounds (column);
    }
}
