#include "PluginProcessor.h"
#include "PluginEditor.h"

PsylanceAudioProcessor::PsylanceAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", psylance::createParameterLayout())
{
}

PsylanceAudioProcessor::~PsylanceAudioProcessor() = default;

void PsylanceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setSampleRate (sampleRate);
    scratchBuffer.setSize (1, juce::jmax (512, samplesPerBlock), false, false, true);
}

void PsylanceAudioProcessor::releaseResources()
{
    engine.allNotesOff();
}

bool PsylanceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet().isDisabled();
}

void PsylanceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto metadata : midiMessages)
        engine.handleMidi (metadata.getMessage());

    const int numSamples = buffer.getNumSamples();
    scratchBuffer.setSize (1, numSamples, false, false, false);
    auto* scratch = scratchBuffer.getWritePointer (0);
    std::fill (scratch, scratch + numSamples, 0.0f);

    psylance::VoiceParams voiceParams;
    voiceParams.cutoff = apvts.getRawParameterValue ("cutoff")->load();
    voiceParams.resonance = apvts.getRawParameterValue ("resonance")->load();
    voiceParams.filterEnvAmount = apvts.getRawParameterValue ("envAmount")->load();
    voiceParams.attack = apvts.getRawParameterValue ("attack")->load();
    voiceParams.decay = apvts.getRawParameterValue ("decay")->load();
    voiceParams.sustain = apvts.getRawParameterValue ("sustain")->load();
    voiceParams.release = apvts.getRawParameterValue ("release")->load();
    voiceParams.subLevel = apvts.getRawParameterValue ("subLevel")->load();
    engine.setParams (voiceParams);

    engine.renderBlock (scratch, numSamples);

    const float gain = apvts.getRawParameterValue ("gain")->load();

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        juce::FloatVectorOperations::multiply (buffer.getWritePointer (channel), scratch, gain, numSamples);
}

juce::AudioProcessorEditor* PsylanceAudioProcessor::createEditor()
{
    return new PsylanceAudioProcessorEditor (*this);
}

void PsylanceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PsylanceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PsylanceAudioProcessor();
}
