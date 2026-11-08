#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/SoundMutation.h"
#include "presets/Presets.h"

#include <cmath>
#include <random>

GnarlAudioProcessor::GnarlAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", gnarl::params::createParameterLayout()),
      rng(std::random_device{}())
{
}

GnarlAudioProcessor::~GnarlAudioProcessor() = default;

void GnarlAudioProcessor::prepareToPlay(double sampleRate, int) {
    engine.setSampleRate(sampleRate);
}

void GnarlAudioProcessor::releaseResources() {
    engine.allNotesOff();
}

gnarl::dsp::VoiceParams GnarlAudioProcessor::makeVoiceParams() const {
    auto get = [this](const char* id) {
        return apvts.getRawParameterValue(id)->load(std::memory_order_relaxed);
    };
    gnarl::dsp::VoiceParams p;
    p.ratio = get("ratio");
    p.modIndex = get("modIndex");
    p.fmEnvDecay = get("fmEnvDecay");
    p.fmEnvAmount = get("fmEnvAmount");
    p.attack = get("attack");
    p.decay = get("decay");
    p.sustain = get("sustain");
    p.release = get("release");
    p.cutoff = get("cutoff");
    p.resonance = get("resonance");
    p.drive = get("drive");
    p.glide = get("glide");
    p.gain = get("gain");
    return p;
}

void GnarlAudioProcessor::writeVoiceParams(const gnarl::dsp::VoiceParams& p) {
    auto set = [this](const char* id, float v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(v));
    };
    set("ratio", p.ratio);
    set("modIndex", p.modIndex);
    set("fmEnvDecay", p.fmEnvDecay);
    set("fmEnvAmount", p.fmEnvAmount);
    set("attack", p.attack);
    set("decay", p.decay);
    set("sustain", p.sustain);
    set("release", p.release);
    set("cutoff", p.cutoff);
    set("resonance", p.resonance);
    set("drive", p.drive);
    set("glide", p.glide);
    set("gain", p.gain);
}

void GnarlAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    auto params = makeVoiceParams();
    float* left = buffer.getWritePointer(0);

    int pos = 0;
    for (const auto metadata : midi) {
        const auto event = metadata.getMessage();
        const int eventPos = std::clamp(metadata.samplePosition, 0, numSamples);
        if (eventPos > pos) {
            engine.render(left + pos, eventPos - pos, params);
            pos = eventPos;
        }
        if (event.isNoteOn())
            engine.noteOn(event.getNoteNumber(), event.getFloatVelocity());
        else if (event.isNoteOff())
            engine.noteOff(event.getNoteNumber());
        else if (event.isAllNotesOff() || event.isAllSoundOff())
            engine.allNotesOff();
    }
    if (pos < numSamples)
        engine.render(left + pos, numSamples - pos, params);

    if (buffer.getNumChannels() > 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* GnarlAudioProcessor::createEditor() {
    return new GnarlAudioProcessorEditor(*this);
}

int GnarlAudioProcessor::getNumPrograms() { return gnarl::presets::count(); }
int GnarlAudioProcessor::getCurrentProgram() { return currentProgram; }

const juce::String GnarlAudioProcessor::getProgramName(int index) {
    return juce::String(gnarl::presets::name(index));
}

void GnarlAudioProcessor::setCurrentProgram(int index) {
    if (index >= 0 && index < getNumPrograms()) {
        currentProgram = index;
        writeVoiceParams(gnarl::presets::factory(index));
    }
}

void GnarlAudioProcessor::mutateSound() {
    auto p = makeVoiceParams();
    gnarl::dsp::mutateSound(p, rng);
    writeVoiceParams(p);
}

void GnarlAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void GnarlAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}
