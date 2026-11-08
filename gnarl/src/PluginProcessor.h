#pragma once

#include <JuceHeader.h>
#include "dsp/FMSynth.h"
#include "params/Parameters.h"

class GnarlAudioProcessor : public juce::AudioProcessor {
public:
    GnarlAudioProcessor();
    ~GnarlAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Biased randomization toward useful darkpsy timbres (Mutate Sound).
    void mutateSound();

    juce::AudioProcessorValueTreeState apvts;

private:
    gnarl::dsp::FMSynth engine;
    gnarl::dsp::VoiceParams makeVoiceParams() const;
    void writeVoiceParams(const gnarl::dsp::VoiceParams& p);

    int currentProgram = 0;
    std::mt19937 rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GnarlAudioProcessor)
};
