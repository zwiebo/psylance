#include "PluginEditor.h"

GnarlAudioProcessorEditor::GnarlAudioProcessorEditor(GnarlAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    auto makeSlider = [this](juce::Slider& s, const juce::String& name) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
        s.setName(name);
        addAndMakeVisible(s);
    };

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem(processorRef.getProgramName(i), i + 1);
    presetBox.setSelectedId(1);
    addAndMakeVisible(presetBox);
    presetBox.onChange = [this] { processorRef.setCurrentProgram(presetBox.getSelectedId() - 1); };

    mutateButton.onClick = [this] { processorRef.mutateSound(); };
    addAndMakeVisible(mutateButton);

    makeSlider(ratioSlider, "Ratio");
    makeSlider(indexSlider, "FM Index");
    makeSlider(fmDecaySlider, "FM Decay");
    makeSlider(attackSlider, "Attack");
    makeSlider(decaySlider, "Decay");
    makeSlider(sustainSlider, "Sustain");
    makeSlider(releaseSlider, "Release");
    makeSlider(cutoffSlider, "Cutoff");
    makeSlider(resonanceSlider, "Reso");
    makeSlider(driveSlider, "Drive");
    makeSlider(glideSlider, "Glide");

    auto attach = [this](juce::Slider& s, const char* id) {
        return std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.apvts, id, s);
    };
    ratioAtt = attach(ratioSlider, "ratio");
    indexAtt = attach(indexSlider, "modIndex");
    fmDecayAtt = attach(fmDecaySlider, "fmEnvDecay");
    attackAtt = attach(attackSlider, "attack");
    decayAtt = attach(decaySlider, "decay");
    sustainAtt = attach(sustainSlider, "sustain");
    releaseAtt = attach(releaseSlider, "release");
    cutoffAtt = attach(cutoffSlider, "cutoff");
    resonanceAtt = attach(resonanceSlider, "resonance");
    driveAtt = attach(driveSlider, "drive");
    glideAtt = attach(glideSlider, "glide");

    setSize(720, 340);
}

void GnarlAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("GNARL", 12, 8, 200, 24, juce::Justification::left);
}

void GnarlAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(10);
    auto top = area.removeFromTop(30);
    presetBox.setBounds(top.removeFromLeft(160));
    mutateButton.setBounds(top.removeFromLeft(130).reduced(2));

    auto row = area;
    const int w = row.getWidth() / 11;
    ratioSlider.setBounds(row.removeFromLeft(w).reduced(4));
    indexSlider.setBounds(row.removeFromLeft(w).reduced(4));
    fmDecaySlider.setBounds(row.removeFromLeft(w).reduced(4));
    attackSlider.setBounds(row.removeFromLeft(w).reduced(4));
    decaySlider.setBounds(row.removeFromLeft(w).reduced(4));
    sustainSlider.setBounds(row.removeFromLeft(w).reduced(4));
    releaseSlider.setBounds(row.removeFromLeft(w).reduced(4));
    cutoffSlider.setBounds(row.removeFromLeft(w).reduced(4));
    resonanceSlider.setBounds(row.removeFromLeft(w).reduced(4));
    driveSlider.setBounds(row.removeFromLeft(w).reduced(4));
    glideSlider.setBounds(row.reduced(4));
}
