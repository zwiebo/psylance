#pragma once

#include "Envelope.h"
#include "Filter.h"
#include "Oscillator.h"
#include "Smoother.h"
#include "Waveshaper.h"

namespace gnarl::dsp {

// Lock-free, allocation-free snapshot of every sound parameter, refreshed by
// the processor once per audio block.
struct VoiceParams {
    float ratio = 1.0f;        // modulator/carrier frequency ratio
    float modIndex = 4.0f;     // FM depth (radians at full envelope)
    float fmEnvDecay = 0.25f;  // FM envelope decay, seconds
    float fmEnvAmount = 1.0f;  // 0..1
    float attack = 0.005f;     // amp ADSR, seconds
    float decay = 0.25f;
    float sustain = 0.8f;
    float release = 0.3f;
    float cutoff = 6000.0f;    // Hz
    float resonance = 0.2f;    // 0..1
    float drive = 0.3f;        // 0..1
    float glide = 0.08f;       // portamento time, seconds (0 = instant)
    float gain = 0.8f;
    float legato = 1.0f;       // >0.5: new note glides without restarting the amp envelope
    int filterMode = 0;        // 0 = low-pass, 1 = band-pass, 2 = high-pass
};

// Monophonic FM voice: carrier + modulator, independent FM envelope, amp
// ADSR, resonant SVF (LP priority, BP/HP selectable), soft-clip drive,
// exponential glide with monophonic legato, per-sample parameter smoothing.
class Voice {
public:
    void setSampleRate(double sr);

    void noteOn(int midiNote, float velocity);
    void noteOff();
    void allNotesOff();

    bool isActive() const { return ampEnv.isActive() || gliding_; }
    int getCurrentNote() const { return currentNote; }

    void render(float* out, int numSamples, const VoiceParams& p);

private:
    static double noteToFreq(int midiNote) {
        return 440.0 * std::exp2((midiNote - 69) / 12.0);
    }

    VoiceParams sanitize(const VoiceParams& p) const;
    static StateVariableFilter::Mode modeFromInt(int m) {
        switch (m) {
            case 1:  return StateVariableFilter::Mode::BandPass;
            case 2:  return StateVariableFilter::Mode::HighPass;
            default: return StateVariableFilter::Mode::LowPass;
        }
    }

    SineOscillator carrier, modulator;
    AdsrEnvelope ampEnv, fmEnv;
    StateVariableFilter filter;
    ParamSmoother ratioSm, indexSm, fmAmtSm, cutoffSm, resoSm, driveSm, gainSm;
    double sampleRate = 44100.0;
    double currentFreq = 440.0, targetFreq = 440.0;
    float velocity = 0.0f;
    float legato_ = 1.0f;
    int currentNote = -1;
    bool gliding_ = false;
    bool freshNote_ = false;
};

// Monophonic synth engine: owns the single lead voice and renders blocks.
// Pure C++, no JUCE — unit-testable without a host.
class FMSynth {
public:
    void setSampleRate(double sr) { voice.setSampleRate(sr); sampleRate = sr; }

    void noteOn(int midiNote, float velocity) { voice.noteOn(midiNote, velocity); }
    void noteOff(int midiNote) {
        if (voice.getCurrentNote() == midiNote || midiNote < 0)
            voice.noteOff();
    }
    void allNotesOff() { voice.allNotesOff(); }

    bool isActive() const { return voice.isActive(); }

    // Adds mono output into out[0..numSamples).
    void render(float* out, int numSamples, const VoiceParams& p) {
        voice.render(out, numSamples, p);
    }

private:
    Voice voice;
    double sampleRate = 44100.0;
};

} // namespace gnarl::dsp
