#pragma once

#include <algorithm>
#include <cmath>

namespace gnarl::dsp {

// Classic ADSR envelope, one gain value per sample. No allocation, no locks.
class AdsrEnvelope {
public:
    void setSampleRate(double sr) { sampleRate = sr > 0.0 ? sr : 44100.0; }

    void setParams(float attackSec, float decaySec, float sustainLevel, float releaseSec) {
        attackSec_ = std::max(attackSec, 1e-4f);
        decaySec_ = std::max(decaySec, 1e-4f);
        sustain_ = std::clamp(sustainLevel, 0.0f, 1.0f);
        releaseSec_ = std::max(releaseSec, 1e-4f);
    }

    void noteOn() {
        stage_ = Stage::Attack;
    }

    void noteOff() {
        if (stage_ != Stage::Off)
            stage_ = Stage::Release;
    }

    void reset() {
        stage_ = Stage::Off;
        level_ = 0.0f;
    }

    bool isActive() const { return stage_ != Stage::Off; }

    // Advance one sample and return the current gain.
    float process() {
        switch (stage_) {
            case Stage::Attack:
                level_ += 1.0f / (attackSec_ * static_cast<float>(sampleRate));
                if (level_ >= 1.0f) {
                    level_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                break;
            case Stage::Decay: {
                const float k = 1.0f - std::exp(-1.0f / (decaySec_ * static_cast<float>(sampleRate)));
                level_ += (sustain_ - level_) * k;
                if (std::abs(level_ - sustain_) < 1e-4f) {
                    level_ = sustain_;
                    stage_ = Stage::Sustain;
                }
                break;
            }
            case Stage::Sustain:
                level_ = sustain_;
                break;
            case Stage::Release: {
                const float k = std::exp(-3.0f / (releaseSec_ * static_cast<float>(sampleRate)));
                level_ *= k;
                if (level_ < 1e-5f) {
                    level_ = 0.0f;
                    stage_ = Stage::Off;
                }
                break;
            }
            case Stage::Off:
                level_ = 0.0f;
                break;
        }
        return level_;
    }

private:
    enum class Stage { Off, Attack, Decay, Sustain, Release };

    double sampleRate = 44100.0;
    float attackSec_ = 0.005f, decaySec_ = 0.25f, sustain_ = 0.8f, releaseSec_ = 0.3f;
    float level_ = 0.0f;
    Stage stage_ = Stage::Off;
};

} // namespace gnarl::dsp
