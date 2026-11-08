# GNARL — Architecture

Project root is `gnarl/`.

## Module map

```
gnarl/
  CMakeLists.txt                 JUCE 8.0.6 pinned via FetchContent
  src/
    PluginProcessor.h/.cpp       AudioProcessor: MIDI in, voice management, state
    PluginEditor.h/.cpp          JUCE editor (Simple mode; Advanced in M3+)
    params/Parameters.h/.cpp     Single authoritative APVTS parameter layout
    dsp/                         Pure C++17 DSP (no JUCE dependency, unit-testable)
      FMSynth.h/.cpp             VoiceParams, mono FM voice + FMSynth engine
      Envelope.h/.cpp            ADSR
      Oscillator.h               Phase-accumulator sine (aliasing-free)
      Filter.h                   TPT state-variable filter (LP/BP/HP)
      Waveshaper.h               tanh soft clipper
      SoundMutation.h/.cpp       Darkpsy-biased parameter mutation (pure function)
    presets/Presets.h/.cpp       Factory presets as VoiceParams
    sequencer/                   M3/M4 contracts (pattern data, scheduler, MIDI export)
  tests/                         Standalone DSP tests (no JUCE needed)
  docs/
```

## Key contracts

- **Audio thread safety**: the audio thread never allocates, locks, does I/O
  or logs. All parameter reads go through `std::atomic<float>` snapshots
  refreshed once per block from the APVTS. DSP classes are plain data +
  `render()` — no allocation, no locks.
- **Single source of truth for parameters**: `params::Parameters` defines the
  APVTS layout; GUI, host automation, presets and mutation all read/write
  through it (via `VoiceParams` snapshots).
- **Separation of concerns**: `sequencer::Pattern` is host-independent data;
  the scheduler (M3) and MIDI export (M4) are separate modules.
- **Determinism**: pattern generation is a pure function of
  (seed, settings, step index).

## Data flow

```
MIDI in ──► FMSynth ──► Voice (FM + envs + filter + drive) ──► stereo out
   │                                                        ▲
   └────────────── sequencer playback (M3) ────────────────┘

GUI ──► APVTS (atomic params) ──► audio thread (lock-free snapshot)
Host state ◄── APVTS copyState / replaceState ──► project save/load
```
