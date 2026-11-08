#pragma once

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

// Topology-preserving transform (TPT) state-variable filter, unconditionally
// stable for all cutoff/resonance settings. Low-pass is the priority mode.
class StateVariableFilter {
public:
    enum class Mode { LowPass, BandPass, HighPass };

    void setSampleRate(double sr) {
        sampleRate = sr > 0.0 ? sr : 44100.0;
        updateCoefficients();
    }

    void setCutoff(double freqHz) {
        cutoff = std::clamp(freqHz, 10.0, 0.45 * sampleRate);
        updateCoefficients();
    }

    // Resonance 0..1; 1 approaches self-oscillation, clamped below it.
    void setResonance(double res) {
        resonance = std::clamp(res, 0.0, 0.98);
        updateCoefficients();
    }

    void setMode(Mode m) { mode = m; }

    void reset() {
        ic1eq = 0.0;
        ic2eq = 0.0;
    }

    float process(float input) {
        const double v3 = input - ic2eq;
        const double v1 = a1 * ic1eq + a2 * v3;
        const double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;

        switch (mode) {
            case Mode::LowPass:  return static_cast<float>(v2);
            case Mode::BandPass: return static_cast<float>(v1);
            case Mode::HighPass: return static_cast<float>(input - k * v1 - v2);
        }
        return static_cast<float>(v2);
    }

private:
    void updateCoefficients() {
        const double g = std::tan(pi * cutoff / sampleRate);
        k = 2.0 * (1.0 - resonance);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a1;
    }

    static constexpr double pi = 3.14159265358979323846;
    double sampleRate = 44100.0;
    double cutoff = 8000.0;
    double resonance = 0.2;
    double g = 0.0, k = 1.6, a1 = 1.0, a2 = 0.0, a3 = 0.0;
    double ic1eq = 0.0, ic2eq = 0.0;
    Mode mode = Mode::LowPass;
};

} // namespace gnarl::dsp
