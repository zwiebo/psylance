add_executable(gnarl_dsp_tests
    dsp_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/FMSynth.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/Envelope.cpp)
add_executable(gnarl_dsp_tests dsp_tests.cpp ${CMAKE_SOURCE_DIR}/src/dsp/FMSynth.cpp ${CMAKE_SOURCE_DIR}/src/dsp/Envelope.cpp)
target_include_directories(gnarl_dsp_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(gnarl_dsp_tests PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gnarl_dsp_tests PRIVATE -Wall -Wextra)
endif()
add_test(NAME dsp COMMAND gnarl_dsp_tests)

add_executable(gnarl_sequencer_tests
    sequencer_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/PatternGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/StepScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/sequencer/MidiExporter.cpp)
target_include_directories(gnarl_sequencer_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(gnarl_sequencer_tests PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gnarl_sequencer_tests PRIVATE -Wall -Wextra)
endif()
add_test(NAME sequencer COMMAND gnarl_sequencer_tests)
