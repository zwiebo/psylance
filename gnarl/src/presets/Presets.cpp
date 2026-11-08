#include "Presets.h"

namespace gnarl::presets {

int count() { return 5; }

const char* name(int index) {
    static const char* names[] = { "Init", "Bubbly Lead", "Dark Growl", "Perc Hit", "Metallic" };
    return index >= 0 && index < count() ? names[index] : "";
}

gnarl::dsp::VoiceParams factory(int index) {
    gnarl::dsp::VoiceParams p;
    switch (index) {
        case 1: // Bubbly Lead
            p.ratio = 2.0f; p.modIndex = 6.0f; p.fmEnvDecay = 0.12f;
            p.attack = 0.003f; p.decay = 0.3f; p.sustain = 0.9f; p.release = 0.25f;
            p.cutoff = 9000.0f; p.resonance = 0.35f; p.drive = 0.2f; p.glide = 0.06f;
            break;
        case 2: // Dark Growl
            p.ratio = 1.0f; p.modIndex = 9.0f; p.fmEnvDecay = 0.5f; p.fmEnvAmount = 0.9f;
            p.attack = 0.01f; p.decay = 0.5f; p.sustain = 0.7f; p.release = 0.4f;
            p.cutoff = 1800.0f; p.resonance = 0.6f; p.drive = 0.7f; p.glide = 0.12f;
            break;
        case 3: // Perc Hit
            p.ratio = 3.0f; p.modIndex = 8.0f; p.fmEnvDecay = 0.05f;
            p.attack = 0.001f; p.decay = 0.12f; p.sustain = 0.0f; p.release = 0.08f;
            p.cutoff = 4000.0f; p.resonance = 0.3f; p.drive = 0.5f; p.glide = 0.0f;
            break;
        case 4: // Metallic
            p.ratio = 4.0f; p.modIndex = 5.0f; p.fmEnvDecay = 0.8f; p.fmEnvAmount = 0.7f;
            p.attack = 0.002f; p.decay = 0.6f; p.sustain = 0.6f; p.release = 0.5f;
            p.cutoff = 12000.0f; p.resonance = 0.5f; p.drive = 0.6f; p.glide = 0.03f;
            break;
        case 0: // Init
        default:
            break;
    }
    return p;
}

} // namespace gnarl::presets
