#pragma once

#include <JuceHeader.h>

namespace gnarl::params {

// Single authoritative definition of every host-automatable parameter.
// GUI, automation, presets and mutation all read/write through this layout.
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace gnarl::params
