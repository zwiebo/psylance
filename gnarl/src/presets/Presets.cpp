#include "Presets.h"

#include "dsp/Patch.h"

namespace gnarl::presets {

int count() { return gnarl::dsp::kNumFactoryPatches; }

const char* name(int index) {
    return index >= 0 && index < count() ? gnarl::dsp::factoryPatch(index).name : "";
}

gnarl::dsp::VoiceParams factory(int index) {
    return gnarl::dsp::factoryPatch(index).params;
}

} // namespace gnarl::presets
