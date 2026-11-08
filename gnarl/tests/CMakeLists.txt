add_executable(gnarl_dsp_tests dsp_tests.cpp ${CMAKE_SOURCE_DIR}/src/dsp/FMSynth.cpp ${CMAKE_SOURCE_DIR}/src/dsp/Envelope.cpp)
target_include_directories(gnarl_dsp_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(gnarl_dsp_tests PRIVATE cxx_std_17)
add_test(NAME dsp COMMAND gnarl_dsp_tests)
