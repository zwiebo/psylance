# GNARL — Architecture

## Module map

```
src/
  PluginProcessor.h/.cpp   AudioProcessor: MIDI in, voice management, state
  PluginEditor.h/.cpp      JUCE editor (Simple / Advanced modes)
  params/Parameters.h/.cpp Single authoritative APVTS parameter layout
  dsp/                     Pure C++17 DSP (no JUCE dependency, unit-testable)
    Envelope.h             ADSR
    Oscillator.h           PolyBLEP sine oscillator (anti-aliased)
    Filter.h               TPT state-variable filter (LP/BP/HP)
    Waveshaper.h           Soft clipper
    Voice.h/.cpp           Mono FM voice: 2 osc + FM env + amp env + filter + drive + glide
    SynthEngine.h/.cpp     Voice allocation, MIDI handling, per-block render
  pattern/                 (M3) host-independent pattern data + generator
  midi/                    (M4) SMF export + VST3 MIDI event output
tests/                     Standalone DSP tests (no JUCE needed)
```

## Key contracts

- **Audio thread safety**: the audio thread never allocates, locks, does I/O
  or logs. All parameter reads go through `std::atomic<float>` snapshots
  refreshed once per block from the APVTS. DSP classes are plain data +
  `process()` — no allocation, no locks.
- **Single source of truth for parameters**: `params::Parameters` defines the
  APVTS layout; GUI, host automation and presets all read/write through it.
- **Separation of concerns**: pattern data structures know nothing about
  audio or MIDI scheduling; the scheduler knows nothing about synthesis.
- **Determinism**: pattern generation is a pure function of
  (seed, settings, step index).

## Data flow

```
MIDI in ──► SynthEngine ──► Voice (FM + envs + filter + drive) ──► stereo out
   │                                                              ▲
   └────────────── pattern playback (M3) ─────────────────────────┘

GUI ──► APVTS (atomic params) ──► audio thread (lock-free snapshot)
Host state ◄── APVTS copyState / replaceState ──► project save/load
```
