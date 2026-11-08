#pragma once

#include <cmath>

namespace gnarl::dsp {

// Phase-accumulator sine oscillator. Sampling sin() directly is exact for a
// sine (no aliasing, unlike naive saw/square generators), so this is both
// simple and correct. Carrier supports phase modulation (FM).
class SineOscillator {
public:
    void setSampleRate(double sr) { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void setFrequency(double freqHz) {
        frequency = freqHz > 0.0 ? freqHz : 0.0;
    }

    void reset() { phase = 0.0; }

    // Plain sine output.
    float process() {
        const float y = static_cast<float>(std::sin(twoPi * phase));
        advance();
        return y;
    }

    // Sine with an added phase-modulation term (radians), used by the FM carrier.
    float processPhaseModulated(double modRadians) {
        const float y = static_cast<float>(std::sin(twoPi * phase + modRadians));
        advance();
        return y;
    }

private:
    void advance() {
        phase += frequency / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;
    }

    static constexpr double twoPi = 6.28318530717958647692;
    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;
};

} // namespace gnarl::dsp
