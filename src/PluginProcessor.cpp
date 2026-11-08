#include "PluginProcessor.h"
#include "PluginEditor.h"

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

int GnarlAudioProcessor::getNumPrograms() { return 5; }
int GnarlAudioProcessor::getCurrentProgram() { return currentProgram; }

const juce::String GnarlAudioProcessor::getProgramName(int index) {
    static const char* names[] = { "Init", "Bubbly Lead", "Dark Growl", "Perc Hit", "Metallic" };
    return index >= 0 && index < 5 ? juce::String(names[index]) : juce::String();
}

void GnarlAudioProcessor::setCurrentProgram(int index) {
    if (index >= 0 && index < getNumPrograms()) {
        currentProgram = index;
        applyPreset(index);
    }
}

void GnarlAudioProcessor::applyPreset(int index) {
    auto set = [this](const char* id, float v) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    switch (index) {
        case 0:
            set("ratio", 1.0f); set("modIndex", 4.0f); set("fmEnvDecay", 0.25f);
            set("fmEnvAmount", 1.0f); set("attack", 0.005f); set("decay", 0.25f);
            set("sustain", 0.8f); set("release", 0.3f); set("cutoff", 6000.0f);
            set("resonance", 0.2f); set("drive", 0.3f); set("glide", 0.08f);
            break;
        case 1: // Bubbly Lead
            set("ratio", 2.0f); set("modIndex", 6.0f); set("fmEnvDecay", 0.12f);
            set("fmEnvAmount", 1.0f); set("attack", 0.003f); set("decay", 0.3f);
            set("sustain", 0.9f); set("release", 0.25f); set("cutoff", 9000.0f);
            set("resonance", 0.35f); set("drive", 0.2f); set("glide", 0.06f);
            break;
        case 2: // Dark Growl
            set("ratio", 1.0f); set("modIndex", 9.0f); set("fmEnvDecay", 0.5f);
            set("fmEnvAmount", 0.9f); set("attack", 0.01f); set("decay", 0.5f);
            set("sustain", 0.7f); set("release", 0.4f); set("cutoff", 1800.0f);
            set("resonance", 0.6f); set("drive", 0.7f); set("glide", 0.12f);
            break;
        case 3: // Perc Hit
            set("ratio", 3.0f); set("modIndex", 8.0f); set("fmEnvDecay", 0.05f);
            set("fmEnvAmount", 1.0f); set("attack", 0.001f); set("decay", 0.12f);
            set("sustain", 0.0f); set("release", 0.08f); set("cutoff", 4000.0f);
            set("resonance", 0.3f); set("drive", 0.5f); set("glide", 0.0f);
            break;
        case 4: // Metallic
            set("ratio", 4.0f); set("modIndex", 5.0f); set("fmEnvDecay", 0.8f);
            set("fmEnvAmount", 0.7f); set("attack", 0.002f); set("decay", 0.6f);
            set("sustain", 0.6f); set("release", 0.5f); set("cutoff", 12000.0f);
            set("resonance", 0.5f); set("drive", 0.6f); set("glide", 0.03f);
            break;
    }
}

void GnarlAudioProcessor::mutateSound() {
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    static const float ratios[] = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    std::uniform_int_distribution<size_t> ratioPick(0, 8);
    std::uniform_real_distribution<float> indexDist(2.0f, 10.0f);
    std::uniform_real_distribution<float> cutoffDist(800.0f, 14000.0f);

    auto set = [this](const char* id, float v) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(v));
    };
    set("ratio", ratios[ratioPick(rng)]);
    set("modIndex", indexDist(rng));
    set("fmEnvDecay", 0.05f + 0.6f * uni(rng));
    set("cutoff", cutoffDist(rng));
    set("drive", 0.1f + 0.7f * uni(rng));
    set("resonance", 0.1f + 0.6f * uni(rng));
    set("glide", 0.02f + 0.2f * uni(rng));
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
