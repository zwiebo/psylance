# GNARL — Validation workflow

Validation is split into **automated** and **manual DAW** tiers. A milestone
is never reported as "FL Studio-tested" because CI compiled it: CI green
means the artifact is well-formed and spec-conformant, not that it works in a
host. Host behavior is only claimed after the manual checklist in
`docs/FL_STUDIO_TESTING.md` passes on a real FL Studio install.

## Tier 1 — Automated unit tests (CI: Linux + Windows)

`ctest` runs two dependency-free suites built from `tests/`:

| Test area | Suite | Covers |
|---|---|---|
| Envelope ADSR | `dsp` | attack peak, release to zero |
| Oscillator | `dsp` | frequency accuracy, finite samples |
| Filter | `dsp` | stability at extreme resonance, LP attenuation |
| Waveshaper | `dsp` | bounded output [-1, 1] |
| Silence on idle | `dsp` | zero output with no notes |
| No stuck notes | `dsp` | release tail decays; `allNotesOff` clears voice |
| Audio sample safety | `dsp` | all samples finite and bounded at 44.1/48/96 kHz, block sizes 64/256/1024 |
| Pattern determinism | `sequencer` | same seed → same pattern; range/density bounds |
| Scheduler timing | `sequencer` | sample-accurate step events at 120 BPM |
| Host tempo changes | `sequencer` | tempo change applies at next step boundary |
| Transport stop | `sequencer` | `stop()` emits note-off, halts; no stuck notes across tempos |
| Glide ties | `sequencer` | tied steps don't emit spurious note-offs |
| MIDI export validity | `sequencer` | SMF format 0 parses; tempo meta; note on/off pairing; 1-bar length |

## Tier 2 — Automated VST3 validation (CI: Windows)

In the **Build GNARL VST3 (Windows)** workflow, after the build:

1. **Bundle structure + metadata** — `tools/verify_vst3_bundle.py` checks
   `Contents/x86_64-win/*.dll`, PE32+ x86-64, VST3 exports
   (`GetPluginFactory`/`InitModule`/`ExitModule`) and embedded metadata
   strings (plugin name, manufacturer). This covers *correct VST3 plugin
   metadata* at the artifact level.
2. **Runtime dependencies** — `tools/check_dll_dependencies.ps1` runs
   `dumpbin /dependents` and fails on any non-system DLL, so the artifact
   loads on a clean Windows machine (with the VC++ Redistributable, see
   `docs/LICENSING.md`).
3. **Steinberg validator** (job `validate-vst3`) — builds the official
   command-line `validator` from the pinned VST3 SDK
   (`steinbergmedia/vst3sdk` tag `v3.8.1_build_84`, MIT) and runs it against
   the artifact: `validator.exe dist/GNARL.vst3`. This checks bundle
   structure, class info, bus arrangements, parameter handling, state
   save/restore and processing — the authoritative spec-conformance gate.

To run the validator manually on a dev machine:

```
git clone --depth 1 --branch v3.8.1_build_84 --recursive https://github.com/steinbergmedia/vst3sdk.git
cmake -B sdk-build -S vst3sdk -DCMAKE_BUILD_TYPE=Release -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF
cmake --build sdk-build --config Release --target validator
sdk-build\bin\Release\validator.exe path\to\GNARL.vst3
```

## Tier 3 — Manual DAW validation (FL Studio, Windows)

Not automatable in CI. Run the full checklist in
`docs/FL_STUDIO_TESTING.md`, which covers: MIDI note-on/note-off, silence on
idle, no stuck notes, audio sample safety (sample rates/buffer sizes),
host tempo changes, transport stop/start, project save/reload, MIDI export
validity (import the exported `.mid` into FL Studio), and plugin metadata as
shown by FL Studio's plugin manager.

## What CI green does and does not mean

| Claim | Evidence required |
|---|---|
| Artifact builds reproducibly | CI build job + `docs/BUILD.md` |
| DSP/sequencer behave correctly | Tier 1 tests |
| Artifact is a well-formed, spec-conformant VST3 | Tier 2 (structure + validator) |
| Plugin loads and works in FL Studio | Tier 3 manual checklist, signed off by the owner |
