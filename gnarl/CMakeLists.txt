cmake_minimum_required(VERSION 3.22)

project(GNARL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pinned dependency: JUCE 8.0.6 (see docs/DECISIONS.md for licensing notes)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GNARL
    COMPANY_NAME            "zwiebo"
    PLUGIN_MANUFACTURER_CODE Zwbo
    PLUGIN_CODE             Gnrl
    FORMATS                 VST3
    PRODUCT_NAME            "GNARL"
    VST3_CATEGORIES         Synth
    AU_MAIN_TYPE            kAudioUnitType_MusicDevice
    BUNDLE_ID               com.zwiebo.gnarl
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(GNARL)

target_sources(GNARL PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/params/Parameters.cpp
    src/dsp/FMSynth.cpp src/dsp/Envelope.cpp src/dsp/SoundMutation.cpp src/presets/Presets.cpp src/sequencer/PatternGenerator.cpp src/sequencer/StepScheduler.cpp src/sequencer/MidiExporter.cpp
    src/dsp/SynthEngine.cpp)

target_include_directories(GNARL PRIVATE src)

target_compile_definitions(GNARL PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_HOST_CROSS_PLATFORM_UID=1
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GNARL PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra)

enable_testing()
add_subdirectory(tests)
