# GNARL — Product Specification

## What is GNARL?

GNARL is a Windows VST3 synthesizer plugin for FL Studio, specialized for
darkpsy / forest psytrance. It combines a gnarly two-operator FM sound engine
with a DAW-tempo-synchronized random pattern generator.

## Target platform

- OS: Windows x64
- DAW: FL Studio
- Format: VST3 (`.vst3` bundle)
- Language: C++17
- Framework: JUCE (pinned, see `docs/DECISIONS.md`)
- Build: CMake + Visual Studio 2022 (MSVC), automated via GitHub Actions

## Capability A — Sound generation (MVP engine)

- Two-operator FM: carrier + modulator, adjustable frequency ratio
- Modulation index with its own independent FM envelope
- Amp ADSR envelope (supports both percussive one-shots and sustained leads)
- Resonant multimode filter (low-pass priority)
- Soft-clip waveshaping distortion
- Monophonic voice handling with glide / portamento
- Preset saving/loading (host + plugin state)
- "Mutate Sound" — biased randomization toward useful darkpsy timbres

## Capability B — Pattern generation (MVP: 16 steps)

- Step sequencer locked to DAW tempo, 16 steps, darkpsy-friendly subdivisions
- Controls: root note, scale, note range, density, velocity variation,
  accent probability, glide probability, random seed, Generate, Play/Stop,
  per-step lock
- Output per step: pitch, note-on/off, velocity, rest, glide/tie, accent
- Deterministic for a given seed + settings
- Internal playback through the synth, MIDI file export, live VST3 MIDI out

## Out of scope for MVP

- Polyphony beyond mono lead behavior
- Elaborate custom graphics
- External/cloud services at runtime

## Success criteria

A real `.vst3` artifact that loads in FL Studio, plays MIDI notes, saves
state with the project, and passes automated DSP tests.
