#include "FMSynth.h"

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

static float finiteOr(float v, float fallback) {
    return std::isfinite(v) ? v : fallback;
}

void Voice::setSampleRate(double sr) {
    sampleRate = sr > 0.0 ? sr : 44100.0;
    carrier.setSampleRate(sampleRate);
    modulator.setSampleRate(sampleRate);
    ampEnv.setSampleRate(sampleRate);
    fmEnv.setSampleRate(sampleRate);
    filter.setSampleRate(sampleRate);
    for (auto* sm : { &ratioSm, &indexSm, &fmAmtSm, &cutoffSm, &resoSm, &driveSm, &gainSm })
        sm->setSampleRate(sampleRate);
}

VoiceParams Voice::sanitize(const VoiceParams& p) const {
    VoiceParams q;
    q.ratio = std::clamp(finiteOr(p.ratio, 1.0f), 0.0625f, 64.0f);
    q.modIndex = std::clamp(finiteOr(p.modIndex, 4.0f), 0.0f, 64.0f);
    q.fmEnvDecay = std::clamp(finiteOr(p.fmEnvDecay, 0.25f), 1e-4f, 10.0f);
    q.fmEnvAmount = std::clamp(finiteOr(p.fmEnvAmount, 1.0f), 0.0f, 1.0f);
    q.attack = std::clamp(finiteOr(p.attack, 0.005f), 1e-4f, 10.0f);
    q.decay = std::clamp(finiteOr(p.decay, 0.25f), 1e-4f, 10.0f);
    q.sustain = std::clamp(finiteOr(p.sustain, 0.8f), 0.0f, 1.0f);
    q.release = std::clamp(finiteOr(p.release, 0.3f), 1e-4f, 10.0f);
    q.cutoff = std::clamp(finiteOr(p.cutoff, 6000.0f), 10.0f,
                          static_cast<float>(0.45 * sampleRate));
    q.resonance = std::clamp(finiteOr(p.resonance, 0.2f), 0.0f, 0.98f);
    q.drive = std::clamp(finiteOr(p.drive, 0.3f), 0.0f, 1.0f);
    q.glide = std::clamp(finiteOr(p.glide, 0.08f), 0.0f, 10.0f);
    q.gain = std::clamp(finiteOr(p.gain, 0.8f), 0.0f, 4.0f);
    q.legato = finiteOr(p.legato, 1.0f) > 0.5f ? 1.0f : 0.0f;
    q.filterMode = std::clamp(p.filterMode, 0, 2);
    return q;
}

void Voice::noteOn(int midiNote, float vel) {
    targetFreq = noteToFreq(midiNote);
    currentNote = midiNote;
    velocity = std::clamp(vel, 0.0f, 1.0f);
    gliding_ = true;
    freshNote_ = true;
    if (legato_ > 0.5f && ampEnv.isActive()) {
        fmEnv.noteOn();
    } else {
        ampEnv.noteOn();
        fmEnv.noteOn();
        filter.reset();
    }
}

void Voice::noteOff() {
    ampEnv.noteOff();
    fmEnv.noteOff();
}

void Voice::allNotesOff() {
    ampEnv.reset();
    fmEnv.reset();
    filter.reset();
    gliding_ = false;
    freshNote_ = false;
    currentNote = -1;
}

void Voice::render(float* out, int numSamples, const VoiceParams& p) {
    const VoiceParams q = sanitize(p);
    legato_ = q.legato;

    ampEnv.setParams(q.attack, q.decay, q.sustain, q.release);
    fmEnv.setParams(q.attack, q.fmEnvDecay, 0.0f, q.fmEnvDecay);
    filter.setMode(modeFromInt(q.filterMode));

    if (freshNote_) {
        ratioSm.snap(q.ratio);
        indexSm.snap(q.modIndex);
        fmAmtSm.snap(q.fmEnvAmount);
        cutoffSm.snap(q.cutoff);
        resoSm.snap(q.resonance);
        driveSm.snap(q.drive);
        gainSm.snap(q.gain);
        freshNote_ = false;
    } else {
        ratioSm.setTarget(q.ratio);
        indexSm.setTarget(q.modIndex);
        fmAmtSm.setTarget(q.fmEnvAmount);
        cutoffSm.setTarget(q.cutoff);
        resoSm.setTarget(q.resonance);
        driveSm.setTarget(q.drive);
        gainSm.setTarget(q.gain);
    }

    const double glideCoeff = q.glide > 1e-4
        ? 1.0 - std::exp(-3.0 / (q.glide * sampleRate))
        : 1.0;

    for (int i = 0; i < numSamples; ++i) {
        currentFreq += (targetFreq - currentFreq) * glideCoeff;
        if (std::abs(currentFreq - targetFreq) < 1e-6)
            gliding_ = false;
        carrier.setFrequency(currentFreq);
        modulator.setFrequency(currentFreq * ratioSm.process());

        filter.setCutoff(cutoffSm.process());
        filter.setResonance(resoSm.process());

        const float fm = fmEnv.process();
        const float amp = ampEnv.process();
        const float modSignal = modulator.process()
            * (indexSm.process() * fmAmtSm.process() * fm);
        float x = carrier.processPhaseModulated(modSignal) * amp * velocity;
        x = filter.process(x);
        x = softClip(x, driveSm.process());
        out[i] += x * gainSm.process();
    }
}

} // namespace gnarl::dsp
