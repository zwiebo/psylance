#pragma once

#include <cmath>

namespace gnarl::dsp {

// One-pole exponential parameter smoother. Removes zipper noise when a
// parameter changes mid-note: a handful of multiply-adds per sample.
class ParamSmoother {
public:
    void setSampleRate(double sr) {
        sampleRate = sr > 0.0 ? sr : 44100.0;
        coeff = 1.0 - std::exp(-1.0 / (timeConstantSec * sampleRate));
    }

    void setTimeConstant(double sec) {
        timeConstantSec = sec > 1e-6 ? sec : 1e-6;
        setSampleRate(sampleRate);
    }

    void snap(float v) { value = v; target = v; }
    void setTarget(float t) { target = t; }

    float process() {
        value += (target - value) * static_cast<float>(coeff);
        return value;
    }

    float current() const { return value; }

private:
    double sampleRate = 44100.0;
    double timeConstantSec = 0.005;
    double coeff = 0.0001;
    float value = 0.0f;
    float target = 0.0f;
};

} // namespace gnarl::dsp
