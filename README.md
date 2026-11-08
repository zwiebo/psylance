# Psylance

Psytrance synth VST3 plugin. Fast MVP: polyphonic saw + sub-sine voices through a
resonant state-variable lowpass filter with a dedicated filter envelope, driven by
MIDI. Built with JUCE 8, C++17, CMake.

## Layout

- `Source/dsp/` — pure DSP, depends only on `juce_core` / `juce_audio_basics`:
  - `Oscillator.h` — saw / square / sine
  - `StateVariableFilter.h` — Chamberlin SVF lowpass
  - `AdsrEnvelope.h` — attack / decay / sustain / release
  - `Voice.h` — osc + sub osc + filter + amp/filter envelopes
  - `SynthEngine.h/.cpp` — 8-voice polyphony with stealing, pitch bend
- `Source/parameters/Parameters.h` — APVTS parameter layout
- `Source/PluginProcessor.*`, `Source/PluginEditor.*` — JUCE glue + minimal UI
- `tests/` — headless JUCE `UnitTest` suite for the DSP

## Build (Visual Studio 2022 / MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Psylance_VST3
```

The VST3 lands in `build/Psylance_artefacts/Release/VST3/`. JUCE is fetched
automatically via CMake `FetchContent` (pinned to tag `8.0.4`); the first
configure clones it, so it needs network access once.

## Tests

```powershell
cmake --build build --config Release --target psylance_tests
ctest --test-dir build --output-on-failure
```

The test binary is a console app and also builds/runs on Linux/macOS
(`cmake -S . -B build && cmake --build build --target psylance_tests && ctest --test-dir build`).

## Parameters

Gain, Cutoff, Resonance, Filter Env amount, Attack, Decay, Sustain, Release, Sub Level.

## Roadmap

- Portamento / glide, more waveforms, distortion drive, presets, richer UI.
