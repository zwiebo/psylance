cmake_minimum_required(VERSION 3.22)

project(Psylance VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PSYLANCE_BUILD_TESTS "Build the headless unit tests" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

add_library(psylance_core INTERFACE)
target_include_directories(psylance_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Source)
target_link_libraries(psylance_core INTERFACE juce::juce_core juce::juce_audio_basics)

juce_add_plugin(Psylance
    COMPANY_NAME            "zwiebo"
    PLUGIN_MANUFACTURER_CODE Zwbo
    PLUGIN_CODE              Psy1
    FORMATS                  VST3 Standalone
    PRODUCT_NAME             "Psylance"
    PLUGIN_DESCRIPTION       "Psytrance synth VST3"
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD  TRUE
)

juce_generate_juce_header(Psylance)

target_sources(Psylance PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/dsp/SynthEngine.cpp
)

target_include_directories(Psylance PRIVATE Source)

target_compile_definitions(Psylance PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_HOST_CROSS_PLATFORM_UID=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Psylance PRIVATE
    psylance_core
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
)

if(PSYLANCE_BUILD_TESTS)
    enable_testing()
    add_executable(psylance_tests
        Source/dsp/SynthEngine.cpp
        tests/TestMain.cpp
        tests/OscillatorTests.cpp
        tests/EnvelopeTests.cpp
        tests/FilterTests.cpp
        tests/VoiceTests.cpp
        tests/SynthEngineTests.cpp
    )
    target_include_directories(psylance_tests PRIVATE tests)
    target_compile_definitions(psylance_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(psylance_tests PRIVATE
        psylance_core
        juce::juce_core
        juce::juce_events
        juce::juce_audio_basics
    )
    add_test(NAME psylance_tests COMMAND psylance_tests)
endif()
