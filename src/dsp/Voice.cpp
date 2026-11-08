#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

void Voice::setSampleRate(double sr) {
    sampleRate = sr > 0.0 ? sr : 44100.0;
    carrier.setSampleRate(sampleRate);
    modulator.setSampleRate(sampleRate);
    ampEnv.setSampleRate(sampleRate);
    fmEnv.setSampleRate(sampleRate);
    filter.setSampleRate(sampleRate);
}

void Voice::noteOn(int midiNote, float vel) {
    targetFreq = noteToFreq(midiNote);
    currentNote = midiNote;
    velocity = std::clamp(vel, 0.0f, 1.0f);
    gliding_ = true;
    ampEnv.noteOn();
    fmEnv.noteOn();
}

void Voice::noteOff() {
    ampEnv.noteOff();
    fmEnv.noteOff();
}

void Voice::allNotesOff() {
    ampEnv.reset();
    fmEnv.reset();
    gliding_ = false;
    currentNote = -1;
}

void Voice::render(float* out, int numSamples, const VoiceParams& p) {
    ampEnv.setParams(p.attack, p.decay, p.sustain, p.release);
    fmEnv.setParams(p.attack, p.fmEnvDecay, 0.0f, p.fmEnvDecay);
    filter.setCutoff(p.cutoff);
    filter.setResonance(p.resonance);
    filter.setMode(StateVariableFilter::Mode::LowPass);

    const double glideCoeff = p.glide > 1e-4
        ? 1.0 - std::exp(-3.0 / (p.glide * sampleRate))
        : 1.0;

    modulator.setFrequency(targetFreq * p.ratio);
    carrier.setFrequency(targetFreq);

    for (int i = 0; i < numSamples; ++i) {
        currentFreq += (targetFreq - currentFreq) * glideCoeff;
        if (std::abs(currentFreq - targetFreq) < 1e-6)
            gliding_ = false;
        carrier.setFrequency(currentFreq);
        modulator.setFrequency(currentFreq * p.ratio);

        const float fm = fmEnv.process();
        const float amp = ampEnv.process();
        const float modSignal = modulator.process()
            * (p.modIndex * p.fmEnvAmount * fm);
        float x = carrier.processPhaseModulated(modSignal) * amp * velocity;
        x = filter.process(x);
        x = softClip(x, p.drive);
        out[i] += x * p.gain;
    }
}

} // namespace gnarl::dsp
