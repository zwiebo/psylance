#include "SynthEngine.h"

namespace psylance
{

void SynthEngine::setSampleRate (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    for (auto& voice : voices)
        voice.setSampleRate (sampleRate);
}

void SynthEngine::setParams (const VoiceParams& newParams)
{
    params = newParams;
    for (auto& voice : voices)
        voice.setParams (params);
}

void SynthEngine::noteOn (int noteNumber, float velocity)
{
    Voice* voice = findFreeVoice();
    voice->noteOn (noteNumber, velocity, currentBend);
}

void SynthEngine::noteOff (int noteNumber)
{
    for (auto& voice : voices)
        if (voice.isActive() && voice.getNote() == noteNumber)
            voice.noteOff();
}

void SynthEngine::allNotesOff()
{
    for (auto& voice : voices)
        voice.noteOff();
}

void SynthEngine::handleMidi (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        noteOn (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        noteOff (message.getNoteNumber());
    else if (message.isPitchWheel())
    {
        const int value = message.getPitchWheelValue();
        currentBend = ((double) value - 8192.0) / 8192.0 * pitchBendRange;
    }
}

void SynthEngine::renderBlock (float* output, int numSamples)
{
    for (auto& voice : voices)
        if (voice.isActive())
            voice.render (output, numSamples);
}

int SynthEngine::activeVoiceCount() const
{
    int count = 0;
    for (const auto& voice : voices)
        if (voice.isActive())
            ++count;
    return count;
}

Voice* SynthEngine::findFreeVoice()
{
    for (auto& voice : voices)
        if (!voice.isActive())
            return &voice;

    Voice* stolen = &voices[(size_t) stealIndex];
    stealIndex = (stealIndex + 1) % maxVoices;
    return stolen;
}

} // namespace psylance
