#pragma once

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

// Classic ADSR envelope, one gain value per sample. No allocation, no locks.
class AdsrEnvelope {
public:
    void setSampleRate(double sr) { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void setParams(float attackSec, float decaySec, float sustainLevel, float releaseSec);
    void noteOn();
    void noteOff();
    void reset();

    bool isActive() const { return stage_ != Stage::Off; }

    // Advance one sample and return the current gain.
    float process();

private:
    enum class Stage { Off, Attack, Decay, Sustain, Release };

    double sampleRate = 44100.0;
    float attackSec_ = 0.005f, decaySec_ = 0.25f, sustain_ = 0.8f, releaseSec_ = 0.3f;
    float level_ = 0.0f;
    Stage stage_ = Stage::Off;
};

} // namespace gnarl::dsp
